/*
 * emulation_impl.h
 *
 *  Created on: December 15, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_IMPL_H_
#define EMULATION_IMPL_H_

#include "config.h"
#include "admissible_log.h"
#include "emulation.h"
#include "packet_impl.h"
#include "../graph-algo/fp_ring.h"
#include "../protocol/topology.h"
#include "../protocol/flags.h"
#include <assert.h>
#include <inttypes.h>

#define MIN(X, Y)	(X <= Y ? X : Y)
#define EMU_ADD_BACKLOG_BATCH_SIZE	64

inline void Emulation::add_backlog(uint16_t src, uint16_t dst, uint16_t flow,
		uint32_t amount, uint16_t start_id, u8* areq_data) {
	uint32_t i, n_pkts;
	struct fp_ring *q_epg_new_pkts;
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);
	assert(flow < FLOWS_PER_NODE);

	q_epg_new_pkts = comm_state.q_epg_new_pkts[src / EMU_ENDPOINTS_PER_EPG];

#ifdef CONFIG_IP_FASTPASS_DEBUG
	printf("adding backlog from %d to %d, amount %d\n", src, dst, amount);
#endif

	/* create and enqueue a packet for each MTU, do this in batches */
	struct emu_packet *pkt_ptrs[EMU_ADD_BACKLOG_BATCH_SIZE];
	while (amount > 0) {
		n_pkts = 0;
		for (i = 0; i < MIN(amount, EMU_ADD_BACKLOG_BATCH_SIZE); i++) {
			pkt_ptrs[n_pkts] = create_packet(src, dst, flow, start_id++,
					areq_data);
			n_pkts++;
			areq_data += emu_req_data_bytes();
		}

		/* enqueue the packets to the correct endpoint group packet queue */
#ifdef DROP_ON_FAILED_ENQUEUE
		if (fp_ring_enqueue_bulk(q_epg_new_pkts, (void **) &pkt_ptrs[0],
				n_pkts) == -ENOBUFS) {
			/* no space in ring. log but don't retry. */
			adm_log_emu_enqueue_backlog_failed(&stat, n_pkts);
			for (i = 0; i < n_pkts; i++)
				free_packet(pkt_ptrs[i], packet_mempool);
		}
#else
		while (fp_ring_enqueue_bulk(q_epg_new_pkts, (void **) &pkt_ptrs[0],
				n_pkts) == -ENOBUFS) {
			/* no space in ring. log and retry. */
			adm_log_emu_enqueue_backlog_failed(&stat, n_pkts);
		}
#endif

		amount -= n_pkts;
	}
}

inline void Emulation::reset_sender(uint16_t src) {
	struct fp_ring *q_resets;
	uint64_t endpoint_id = src;

	q_resets = comm_state.q_resets[src / EMU_ENDPOINTS_PER_EPG];

	/* enqueue a notification to the endpoint driver's reset queue */
	/* cast src id to a pointer */
	while (fp_ring_enqueue(q_resets, (void *) endpoint_id) == -ENOBUFS) {
		/* no space in ring. this should never happen. log and retry. */
		adm_log_emu_enqueue_reset_failed(&stat);
	}
}

#endif /* EMULATION_IMPL_H_ */
