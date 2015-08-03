/*
 * EndpointDriver.cc
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#include "EndpointDriver.h"

#include <assert.h>
#include <stdint.h>
#include "../config.h"
#include "../endpoint_group.h"
#include "../emulation.h"
#include "../packet_impl.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#define EPG_MAX_BURST	64

EndpointDriver::EndpointDriver(struct fp_ring* q_new_packets,
		struct fp_ring* q_to_router, struct fp_ring* q_from_router,
		struct fp_ring *q_resets, EndpointGroup* epg,
		struct fp_mempool *packet_mempool, uint32_t burst_size)
	: m_q_new_packets(q_new_packets),
	  m_q_to_router(q_to_router),
	  m_q_from_router(q_from_router),
	  m_q_resets(q_resets),
	  m_epg(epg),
	  m_cur_time(0),
	  m_packet_mempool(packet_mempool),
	  m_burst_size(burst_size)
{}

void EndpointDriver::assign_to_core(EmulationOutput *out, Dropper *dropper,
		struct emu_admission_core_statistics *stat, uint16_t core_index) {
	m_epg->assign_to_core(out);
	m_dropper = dropper;
	m_stat = stat;
	m_core_index = core_index;
}

void EndpointDriver::cleanup() {
	free_packet_ring(m_q_from_router, m_packet_mempool);

	delete m_epg;
}

void EndpointDriver::step() {
	uint64_t endpoint_id;

	/* handle any resets */
	while (fp_ring_dequeue(m_q_resets, (void **) &endpoint_id) != -ENOENT) {
		/* cast pointer to int identifying the endpoint */
		m_epg->reset((uint16_t) endpoint_id);
	}

	push();
	pull();
	process_new();

	m_cur_time++;
}

/**
 * Emulate push at a single endpoint group with index @index
 */

inline void EndpointDriver::push() {
	uint32_t n_pkts;
	struct emu_packet *pkts[EPG_MAX_BURST];

	adm_log_emu_endpoint_driver_push_begin(m_stat);

	/* dequeue packets from network, pass to endpoint group */
	n_pkts = fp_ring_dequeue_burst(m_q_from_router, (void **) &pkts[0],
			m_burst_size);
	m_epg->push_batch(&pkts[0], n_pkts);

	adm_log_emu_endpoint_driver_pushed(m_stat, n_pkts);

#ifdef CONFIG_IP_FASTPASS_DEBUG
	printf("EndpointDriver on core %d pushed %d packets\n", m_core_index,
			n_pkts);
#endif
}

/**
 * Emulate pull at a single endpoint group with index @index
 */
inline void EndpointDriver::pull() {
	uint32_t n_pkts, i;
	struct emu_packet *pkts[EPG_MAX_BURST];

	adm_log_emu_endpoint_driver_pull_begin(m_stat);

	/* pull a batch of packets from the epg, enqueue to router */
	n_pkts = m_epg->pull_batch(&pkts[0], m_burst_size, m_cur_time, m_dropper);
	assert(n_pkts <= m_burst_size);
#ifdef DROP_ON_FAILED_ENQUEUE
	if (n_pkts > 0 && fp_ring_enqueue_bulk(m_q_to_router,
			(void **) &pkts[0], n_pkts) == -ENOBUFS) {
		/* no space in ring. log but don't retry. */
		adm_log_emu_send_packets_failed(m_stat, n_pkts);
		free_packet_bulk(&pkts[0], m_packet_mempool, n_pkts);
	} else {
		adm_log_emu_endpoint_driver_pulled(m_stat, n_pkts);
	}
#else
	while (n_pkts > 0 && fp_ring_enqueue_bulk(m_q_to_router,
			(void **) &pkts[0], n_pkts) == -ENOBUFS) {
		/* no space in ring. log and retry. */
		adm_log_emu_send_packets_failed(m_stat, n_pkts);
	}
	adm_log_emu_endpoint_sent_packets(m_stat, n_pkts);
	adm_log_emu_endpoint_driver_pulled(m_stat, n_pkts);
#endif

#ifdef CONFIG_IP_FASTPASS_DEBUG
	printf("EndpointDriver on core %d pulled %d packets\n", m_core_index,
			n_pkts);
#endif
}

/**
 * Emulate new packets at a single endpoint group with index @index
 */
inline void EndpointDriver::process_new()
{
	uint32_t n_pkts;
	struct emu_packet *pkts[EPG_MAX_BURST];

	adm_log_emu_endpoint_driver_new_begin(m_stat);

	/* dequeue new packets, pass to endpoint group */
	n_pkts = fp_ring_dequeue_burst(m_q_new_packets, (void **) &pkts,
			m_burst_size);
	m_epg->new_packets(&pkts[0], n_pkts, m_cur_time, m_dropper);
	adm_log_emu_endpoint_driver_processed_new(m_stat, n_pkts);

#ifdef CONFIG_IP_FASTPASS_DEBUG
	printf("EndpointDriver on core %d processed %d new packets\n",
			m_core_index, n_pkts);
#endif
}
