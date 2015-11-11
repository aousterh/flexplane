/*
 * emulation.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "admitted.h"
#include "emulation_core.h"
#include "emu_comm_core_map.h"
#include "endpoint_group.h"
#include "router.h"
#include "drivers/EndpointDriver.h"
#include "drivers/RouterDriver.h"
#include "output.h"
#include "packet_impl.h"
#include "util/make_mempool.h"
#include "util/make_ring.h"
#include "../protocol/topology.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <assert.h>
#include <stdio.h>

Emulation::Emulation(struct fp_mempool **admitted_traffic_mempool,
		struct fp_ring **q_admitted_out, uint32_t packet_ring_size,
		RouterType r_type, void *r_args, EndpointType e_type, void *e_args,
		struct emu_topo_config *m_topo_config)
	: m_topo_config(m_topo_config) {
	uint32_t i, pq;
	EndpointGroup	*epgs[EMU_MAX_ENDPOINT_GROUPS];
	Router			*rtrs[EMU_MAX_ROUTERS];
	EmulationOutput *out;
	Dropper *dropper;
	char s[64];
	struct fp_ring *packet_queues[EMU_MAX_PACKET_QS];

	/* create packet mempool */
	m_packet_mempool = make_mempool("packet_mempool", PACKET_MEMPOOL_SIZE,
			EMU_ALIGN(sizeof(struct emu_packet)), PACKET_MEMPOOL_CACHE_SIZE, 0,
			0);

	/* copy pointers to mempools */
	for (i = 0; i < N_COMM_CORES; i++)
		m_admitted_traffic_mempool.push_back(admitted_traffic_mempool[i]);

	/* copy pointers to admitted outs */
	for (i = 0; i < ALGO_N_CORES; i++)
		m_q_admitted_out.push_back(q_admitted_out[i]);

	/* init packet_queues */
	pq = 0;
	for (i = 0; i < num_packet_qs(m_topo_config); i++) {
		snprintf(s, sizeof(s), "packet_q_%d", i);
		if (i < 2 * num_endpoint_groups(m_topo_config)) {
			/* for comm -> epgs communication and resets, single producer */
			packet_queues[i] = make_ring(s, packet_ring_size, 0,
					RING_F_SP_ENQ | RING_F_SC_DEQ);
		} else {
			packet_queues[i] = make_ring(s, packet_ring_size, 0,
					RING_F_SC_DEQ);
		}
	}

	/* initialize log to zeroes */
	memset(&m_stat, 0, sizeof(struct emu_admission_statistics));

	/* initialize state used to communicate with comm cores */
	for (i = 0; i < num_endpoint_groups(m_topo_config); i++) {
		m_comm_state.q_epg_new_pkts[i] = packet_queues[pq++];
		m_comm_state.q_resets[i] = packet_queues[pq++];
	}

	/* initialize the topology */
	construct_topology(&epgs[0], &rtrs[0], r_type, r_args, e_type, e_args);

	/* assign endpoints and routers to cores */
	assign_components_to_cores(&epgs[0], &rtrs[0], &packet_queues[pq]);

	/* get queue bank stat pointers from routers */
	for (i = 0; i < num_routers(m_topo_config); i++) {
		m_queue_bank_stats.push_back(rtrs[i]->get_queue_bank_stats());
	}

	/* get port drop stat pointers from cores */
	for (i = 0; i < ALGO_N_CORES; i++) {
		m_port_drop_stats.push_back(m_cores[i]->get_port_drop_stats());
	}
}

void Emulation::step() {
	uint32_t i;

	for (i = 0; i < ALGO_N_CORES; i++)
		m_cores[i]->step();
}

void Emulation::cleanup() {
	uint32_t i;
	struct emu_admitted_traffic *admitted;

	/* cleanup cores */
	for (i = 0; i < ALGO_N_CORES; i++) {
		m_cores[i]->cleanup();
		delete m_cores[i];
	}

	/* free queues to comm core */
	for (i = 0; i < num_endpoint_groups(m_topo_config); i++) {
		/* free packet queues, return packets to mempool */
		free_packet_ring(m_comm_state.q_epg_new_pkts[i], m_packet_mempool);
		free_packet_ring(m_comm_state.q_resets[i], m_packet_mempool);
	}

	/* empty queues of admitted traffic, return structs to the mempool */
	for (i = 0; i < m_q_admitted_out.size(); i++) {
		while (fp_ring_dequeue(m_q_admitted_out[i], (void **) &admitted) == 0)
			/* TODO: free these admitted structs */
		fp_free(m_q_admitted_out[i]);
	}

	for (i = 0; i < m_admitted_traffic_mempool.size(); i++)
		fp_free(m_admitted_traffic_mempool[i]);
	fp_free(m_packet_mempool);
}

/* configure the topology of endpoints and routers */
void Emulation::construct_topology(EndpointGroup **epgs, Router **rtrs,
		RouterType r_type, void *r_args, EndpointType e_type, void *e_args) {
	uint32_t i, rtr_index;

	printf("constructing topology with %d routers and %d endpoints\n",
			num_routers(m_topo_config), num_endpoints(m_topo_config));

	/* initialize endpoints */
	for (i = 0; i < num_endpoint_groups(m_topo_config); i++) {
		epgs[i] = EndpointGroupFactory::NewEndpointGroup(e_type,
				i * endpoints_per_rack(m_topo_config), e_args, m_topo_config);
		assert(epgs[i] != NULL);
	}

	/* initialize Tors */
	for (rtr_index = 0; rtr_index < num_tors(m_topo_config); rtr_index++) {
		rtrs[rtr_index] = RouterFactory::NewRouter(r_type, r_args, TOR_ROUTER,
				rtr_index, m_topo_config);
		assert(rtrs[rtr_index] != NULL);
	}

	/* initialize the Core */
	if (num_core_routers(m_topo_config) > 0) {
		rtrs[rtr_index] = RouterFactory::NewRouter(r_type, r_args, CORE_ROUTER,
				num_tors(m_topo_config), m_topo_config);
		assert(rtrs[rtr_index] != NULL);
	}
}

/* Populate rtr_masks with a mask for each set of ports that faces a different
 * neighbor, for a ToR. */
void Emulation::set_tor_port_masks(uint64_t *rtr_masks) {
	if (num_core_routers(m_topo_config) == 0) {
		/* single rack topology */
		if (endpoints_per_rack(m_topo_config) <= 32)
			rtr_masks[0] = 0xFFFFFFFF; /* 32 ports */
		else
			rtr_masks[0] = 0xFFFFFFFFFFFFFFFF; /* 64 ports */
	} else {
		/* multiple racks - half of ports face up, half face down */
		rtr_masks[0] = 0xFFFFFFFF;
		rtr_masks[1] = 0xFFFFFFFF00000000;
	}
}

/* Populate rtr_masks with a mask for each set of ports that faces a different
 * neighbor, for a core router. */
void Emulation::set_core_port_masks(uint64_t *rtr_masks) {
	uint16_t i;

	if (num_core_routers(m_topo_config) == 0) {
		throw std::runtime_error("no core router in this topology");
	}

	if (num_tors(m_topo_config) == 2) {
		/* two neighbors - half of ports face each */
		rtr_masks[0] = 0xFFFFFFFF;
		rtr_masks[1] = 0xFFFFFFFF00000000;
	} else if (num_tors(m_topo_config) <= 4) {
		/* three or four neighbors - 16 ports for each */
		for (i = 0; i < num_tors(m_topo_config); i++)
			rtr_masks[i] = 0xFFFFULL << (16 * i);
	} else if (num_tors(m_topo_config) <= 8) {
		/* five to eight neighbors - 8 ports each */
		for (i = 0; i < num_tors(m_topo_config); i++)
			rtr_masks[i] = 0xFFULL << (i * 8);
	} else {
		throw std::runtime_error("this number of ToRs is not yet supported");
	}
}

/* Construct drivers and map them to cores based on number of cores available.
 * */
void Emulation::assign_components_to_cores(EndpointGroup **epgs, Router **rtrs,
		struct fp_ring **packet_queues) {
	uint16_t core_index = 0;
	uint16_t i;
	uint32_t pq, rtr_index;
	struct fp_ring *q_epg_ingress[EMU_MAX_ENDPOINT_GROUPS];
	struct fp_ring *q_router_ingress[EMU_MAX_ROUTERS];
	struct fp_ring *q_router_egress[EMU_MAX_OUTPUTS_PER_RTR];
	uint64_t rtr_masks[EMU_MAX_OUTPUTS_PER_RTR];
	EndpointDriver	*epg_drivers[EMU_MAX_ENDPOINT_GROUPS];
	RouterDriver	*router_drivers[EMU_MAX_ROUTERS];
	void *p_aligned; /* all memory must be aligned to 64-byte cache lines */

	/* First, construct drivers */

	/* setup rings for routers and endpoints */
	pq = 0;
	for (i = 0; i < num_endpoint_groups(m_topo_config); i++)
		q_epg_ingress[i] = packet_queues[pq++];
	for (i = 0; i < num_routers(m_topo_config); i++)
		q_router_ingress[i] = packet_queues[pq++];

	/* initialize all the endpoint drivers */
	for (i = 0; i < num_endpoint_groups(m_topo_config); i++) {
		p_aligned = fp_malloc("EndpointDriver", sizeof(class EndpointDriver));
		epg_drivers[i] =
				new (p_aligned) EndpointDriver(m_comm_state.q_epg_new_pkts[i],
						q_router_ingress[i], q_epg_ingress[i],
						m_comm_state.q_resets[i], epgs[i], m_packet_mempool,
						endpoints_per_rack(m_topo_config));
	}

	/* initialize the drivers for the ToRs */
	set_tor_port_masks(&rtr_masks[0]);
	if (tor_neighbors(m_topo_config) == 2)
		q_router_egress[1] = q_router_ingress[num_tors(m_topo_config)];

	for (rtr_index = 0; rtr_index < num_tors(m_topo_config); rtr_index++) {
		q_router_egress[0] = q_epg_ingress[rtr_index];
		p_aligned = fp_malloc("RouterDriver", sizeof(class RouterDriver));
		router_drivers[rtr_index] =
				new (p_aligned) RouterDriver(rtrs[rtr_index],
						q_router_ingress[rtr_index], &q_router_egress[0],
						&rtr_masks[0], tor_neighbors(m_topo_config),
						m_packet_mempool, endpoints_per_rack(m_topo_config));
	}

	/* initialize the Core's driver */
	if (num_core_routers(m_topo_config) > 0) {
		set_core_port_masks(&rtr_masks[0]);
		p_aligned = fp_malloc("RouterDriver", sizeof(class RouterDriver));
		router_drivers[rtr_index] =
				new (p_aligned) RouterDriver(rtrs[rtr_index],
						q_router_ingress[rtr_index], &q_router_ingress[0],
						&rtr_masks[0], core_neighbors(m_topo_config),
						m_packet_mempool, endpoints_per_rack(m_topo_config));
	}


	/* Now assign drivers to cores */
	if (ALGO_N_CORES == (num_routers(m_topo_config) +
			num_endpoint_groups(m_topo_config))) {
		/* put 1 router or endpoint group on each core */
		for (i = 0; i < num_endpoint_groups(m_topo_config); i++) {
			p_aligned = fp_malloc("EmulationCore", sizeof(class EmulationCore));
			m_cores[core_index] =
					new (p_aligned) EmulationCore(&epg_drivers[i], NULL, 1, 0,
							core_index, m_q_admitted_out[core_index],
							m_admitted_traffic_mempool[comm_for_emu(
									core_index)], m_packet_mempool);
			core_index++;
		}
		for (i = 0; i < num_routers(m_topo_config); i++) {
			p_aligned = fp_malloc("EmulationCore", sizeof(class EmulationCore));
			m_cores[core_index] = new (p_aligned) EmulationCore(NULL,
					&router_drivers[i], 0, 1, core_index,
					m_q_admitted_out[core_index],
					m_admitted_traffic_mempool[comm_for_emu(core_index)],
					m_packet_mempool);
			core_index++;
		}
	} else if (ALGO_N_CORES == num_routers(m_topo_config)) {
		/* 1 epg + 1 rtr on first num_racks cores, core router on last core */
		for (i = 0; i < m_topo_config->num_racks; i++) {
			p_aligned = fp_malloc("EmulationCore", sizeof(class EmulationCore));
			m_cores[core_index] =
					new (p_aligned) EmulationCore(&epg_drivers[i],
							&router_drivers[i], 1, 1, core_index,
							m_q_admitted_out[core_index],
							m_admitted_traffic_mempool[comm_for_emu(
									core_index)], m_packet_mempool);
			core_index++;
		}
		if (num_core_routers(m_topo_config) == 1) {
			p_aligned = fp_malloc("EmulationCore", sizeof(class EmulationCore));
			m_cores[core_index] = new (p_aligned) EmulationCore(NULL,
					&router_drivers[core_index], 0, 1, core_index,
					m_q_admitted_out[core_index],
					m_admitted_traffic_mempool[comm_for_emu(core_index)],
					m_packet_mempool);
		}
	} else if (ALGO_N_CORES == 1) {
		/* assign everything to one core */
		p_aligned = fp_malloc("EmulationCore", sizeof(class EmulationCore));
		m_cores[core_index] = new (p_aligned) EmulationCore(epg_drivers,
				router_drivers, num_endpoint_groups(m_topo_config),
				num_routers(m_topo_config), core_index,
				m_q_admitted_out[core_index],
				m_admitted_traffic_mempool[comm_for_emu(core_index)],
				m_packet_mempool);
	} else {
		throw std::runtime_error("no specified way to assign this topology to cores");
	}

	/* Set stats pointers */
	for (i = 0; i < ALGO_N_CORES; i++)
		m_core_stats[i] = m_cores[i]->stats();
}
