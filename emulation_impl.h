/*
 * emulation_impl.h
 *
 *  Created on: December 15, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_IMPL_H_
#define EMULATION_IMPL_H_

#include "config.h"
#include "api.h"
#include "api_impl.h"
#include "admissible_log.h"
#include "emulation.h"
#include "packet.h"
#include "../graph-algo/fp_ring.h"
#include "../protocol/topology.h"
#include "../protocol/flags.h"
#include <assert.h>
#include <inttypes.h>

#define MIN(X, Y)	(X <= Y ? X : Y)
#define EMU_ADD_BACKLOG_BATCH_SIZE	64

static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount, uint16_t start_id, u8* areq_data) {
	uint32_t i, n_pkts;
	struct emu_comm_state *comm_state;
	struct fp_ring *q_epg_new_pkts;
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);
	assert(flow < FLOWS_PER_NODE);

	comm_state = &state->comm_state;
	q_epg_new_pkts = comm_state->q_epg_new_pkts[src / EMU_ENDPOINTS_PER_EPG];

	/* create and enqueue a packet for each MTU, do this in batches */
	struct emu_packet *pkt_ptrs[EMU_ADD_BACKLOG_BATCH_SIZE];
	while (amount > 0) {
		n_pkts = 0;
		for (i = 0; i < MIN(amount, EMU_ADD_BACKLOG_BATCH_SIZE); i++) {
			pkt_ptrs[n_pkts] = create_packet(state, src, dst, flow,
					start_id++, areq_data);
			n_pkts++;
			areq_data += emu_req_data_bytes();
		}

		/* enqueue the packets to the correct endpoint group packet queue */
		while (fp_ring_enqueue_bulk(q_epg_new_pkts, (void **) &pkt_ptrs[0],
				n_pkts) == -ENOBUFS) {
			/* no space in ring. log and retry. */
			adm_log_emu_enqueue_backlog_failed(&state->stat, n_pkts);
		}

		amount -= n_pkts;
	}
}

static inline
void emu_reset_sender(struct emu_state *state, uint16_t src) {
	struct emu_comm_state *comm_state;
	struct fp_ring *q_resets;
	uint64_t endpoint_id = src;

	comm_state = &state->comm_state;
	q_resets = comm_state->q_resets[src / EMU_ENDPOINTS_PER_EPG];

	/* enqueue a notification to the endpoint driver's reset queue */
	/* cast src id to a pointer */
	while (fp_ring_enqueue(q_resets, (void *) endpoint_id) == -ENOBUFS) {
		/* no space in ring. this should never happen. log and retry. */
		adm_log_emu_enqueue_reset_failed(&state->stat);
	}
}

#endif /* EMULATION_IMPL_H_ */
