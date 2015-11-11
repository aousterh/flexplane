/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "admissible_log.h"
#include "config.h"
#include "endpoint.h"
#include "emu_topology.h"
#include "packet.h"
#include "packet_impl.h"
#include "router.h"
#include "queue_bank_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "../protocol/topology.h"
#include <assert.h>
#include <inttypes.h>
#include <vector>

#define ADMITTED_MEMPOOL_SIZE			128
#define ADMITTED_Q_LOG_SIZE				4
#define PACKET_MEMPOOL_SIZE				(1024 * 16)
#define	PACKET_MEMPOOL_CACHE_SIZE		256
#define PACKET_Q_LOG_SIZE				10
#define MIN(X, Y)						(X <= Y ? X : Y)
#define EMU_ADD_BACKLOG_BATCH_SIZE		64
#define BACKLOG_PREFETCH_OFFSET			3

class EmulationCore;
class EndpointGroup;
class Router;
class EmulationOutput;
class EndpointDriver;
class RouterDriver;

/**
 * Emu state allocated for each comm core
 * @q_epg_new_pkts: queues of packets from comm core to each endpoint group
 * @q_resets: a queue of pending resets, for each endpoint group
 */
struct emu_comm_state {
	struct fp_ring	*q_epg_new_pkts[EMU_MAX_EPGS_PER_COMM];
	struct fp_ring	*q_resets[EMU_MAX_EPGS_PER_COMM];
};

/**
 * Class to store the global state of the emulation.
 */
class Emulation {
public:
	Emulation(struct fp_mempool **admitted_traffic_mempool,
			struct fp_ring **q_admitted_out, uint32_t packet_ring_size,
			enum RouterType r_type, void *r_args, enum EndpointType e_type,
			void *e_args, struct emu_topo_config *topo_config);

	/**
	 * Run the emulation for one step.
	 */
	void step();

	/**
	 * Add backlog from @src to @dst for @flow. Add @amount MTUs, with the
	 * 	first id of @start_id. @areq_data provides additional information about
	 * 	each MTU.
	 */
	inline void add_backlog(uint16_t src, uint16_t dst, uint16_t flow,
			uint32_t amount, uint16_t start_id, u8* areq_data);

	/**
	 * Reset the emulation state for a single sender @src.
	 */
	inline void reset_sender(uint16_t src);

	/**
	 * Cleanup before destroying this Emulation.
	 */
	void cleanup();

private:
	/**
	 * Creates a packet, returns a pointer to the packet.
	 */
	inline struct emu_packet *create_packet(uint16_t src, uint16_t dst,
			uint16_t flow, uint16_t id, uint8_t *areq_data);

	/**
	 * Creates a batch of packets, putting pointers to them in pkt_ptrs.
	 */
	inline uint32_t create_packet_batch(struct emu_packet **pkt_ptrs,
			uint16_t src, uint16_t dst, uint16_t flow, uint16_t start_id,
			uint32_t amount, uint8_t *areq_data);

	/**
	 * Creates all of the routers and endpoint groups in the network.
	 */
	void construct_topology(EndpointGroup **epgs, Router **rtrs,
			RouterType r_type, void *r_args, EndpointType e_type,
			void *e_args);

	/**
	 * Assign the emulated components to the hardware cores.
	 */
	void assign_components_to_cores(EndpointGroup **epgs, Router **rtrs,
			struct fp_ring **packet_queues);

	void set_tor_port_masks(uint64_t *rtr_masks);
	void set_core_port_masks(uint64_t *rtr_masks);

	/* Public variables for easy access from the log and emulation cores. */
public:
	struct emu_admission_statistics			m_stat;
	struct emu_admission_core_statistics	*m_core_stats[ALGO_N_CORES];
	EmulationCore							*m_cores[ALGO_N_CORES];
	std::vector<struct queue_bank_stats*>	m_queue_bank_stats;
	std::vector<struct port_drop_stats*>	m_port_drop_stats;

private:
	struct fp_mempool						*m_packet_mempool;
	std::vector<struct fp_mempool *>		m_admitted_traffic_mempool;
	std::vector<struct fp_ring *>			m_q_admitted_out;
	struct emu_comm_state					m_comm_state;
	struct emu_topo_config					*m_topo_config;
};


inline void Emulation::add_backlog(uint16_t src, uint16_t dst, uint16_t flow,
		uint32_t amount, uint16_t start_id, u8* areq_data) {
	uint32_t amount_this_iter, amount_created;
	struct fp_ring *q_epg_new_pkts;
	assert(src < num_endpoints(m_topo_config));
	assert(dst < num_endpoints(m_topo_config));
	assert(flow < FLOWS_PER_NODE);

	q_epg_new_pkts =
			m_comm_state.q_epg_new_pkts[src / endpoints_per_epg(m_topo_config)];

#ifdef CONFIG_IP_FASTPASS_DEBUG
	printf("adding backlog from %d to %d, amount %d\n", src, dst, amount);
#endif

	/* create and enqueue a packet for each MTU, do this in batches */
	struct emu_packet *pkt_ptrs[EMU_ADD_BACKLOG_BATCH_SIZE];
	while (amount > 0) {
		amount_this_iter = MIN(amount, EMU_ADD_BACKLOG_BATCH_SIZE);

		/* this might return 0 if using DROP_ON_FAILED_ENQUEUE and the packet
		 * pool has been exhausted */
		amount_created = create_packet_batch(&pkt_ptrs[0], src, dst, flow,
				start_id, amount_this_iter, areq_data);

		/* enqueue the packets to the correct endpoint group packet queue */
#ifdef DROP_ON_FAILED_ENQUEUE
		if ((amount_created != 0) &&
				fp_ring_enqueue_bulk(q_epg_new_pkts, (void **) &pkt_ptrs[0],
				amount_this_iter) == -ENOBUFS) {
			/* no space in ring. log but don't retry. */
			adm_log_emu_enqueue_backlog_failed(&m_stat, amount_this_iter);
			free_packet_bulk(&pkt_ptrs[0], m_packet_mempool, amount_this_iter);
		}
#else
		while (fp_ring_enqueue_bulk(q_epg_new_pkts, (void **) &pkt_ptrs[0],
				amount_this_iter) == -ENOBUFS) {
			/* no space in ring. log and retry. */
			adm_log_emu_enqueue_backlog_failed(&m_stat, amount_this_iter);
		}
#endif

		amount -= amount_this_iter;
		start_id += amount_this_iter;
	}
}

inline void Emulation::reset_sender(uint16_t src) {
	struct fp_ring *q_resets;
	uint64_t endpoint_id = src;

	q_resets =
			m_comm_state.q_resets[src / endpoints_per_epg(m_topo_config)];

	/* enqueue a notification to the endpoint driver's reset queue */
	/* cast src id to a pointer */
	while (fp_ring_enqueue(q_resets, (void *) endpoint_id) == -ENOBUFS) {
		/* no space in ring. this should never happen. log and retry. */
		adm_log_emu_enqueue_reset_failed(&m_stat);
	}
}

inline struct emu_packet *Emulation::create_packet(uint16_t src, uint16_t dst,
		uint16_t flow, uint16_t id, uint8_t *areq_data)
{
	struct emu_packet *packet;

	/* allocate a packet */
	while (fp_mempool_get(m_packet_mempool, (void **) &packet) == -ENOENT) {
		adm_log_emu_packet_alloc_failed(&m_stat);
	}
	packet_init(packet, src, dst, flow, id, areq_data);

	return packet;
}

inline uint32_t Emulation::create_packet_batch(struct emu_packet **pkt_ptrs,
		uint16_t src, uint16_t dst, uint16_t flow, uint16_t start_id,
		uint32_t amount, uint8_t *areq_data)
{
	uint32_t i;

	/* fetch a batch of packets */
#ifdef DROP_ON_FAILED_ENQUEUE
	if (fp_mempool_get_bulk(m_packet_mempool, (void **) pkt_ptrs, amount)
			== -ENOENT) {
		adm_log_emu_packet_alloc_failed(&m_stat);

		/* failed to alloc packets */
		areq_data += emu_req_data_bytes() * amount;
		return 0;
	}
#else
	while (fp_mempool_get_bulk(m_packet_mempool, (void **) pkt_ptrs, amount)
			== -ENOENT) {
		adm_log_emu_packet_alloc_failed(&m_stat);
	}
#endif

	/* initialize all packets, using prefetching */
	for (i = 0; i < BACKLOG_PREFETCH_OFFSET && i < amount; i++)
		fp_prefetch0(pkt_ptrs[i]);

	for (i = 0; i + BACKLOG_PREFETCH_OFFSET < amount; i++) {
		fp_prefetch0(pkt_ptrs[i + BACKLOG_PREFETCH_OFFSET]);
		packet_init(pkt_ptrs[i], src, dst, flow, start_id++, areq_data);
		areq_data += emu_req_data_bytes();
	}

	for (; i < amount; i++) {
		packet_init(pkt_ptrs[i], src, dst, flow, start_id++, areq_data);
		areq_data += emu_req_data_bytes();
	}

	return amount;
}

#endif /* EMULATION_H_ */
