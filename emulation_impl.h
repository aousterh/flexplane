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
#include <assert.h>
#include <inttypes.h>

#define MIN(X, Y)	(X <= Y ? X : Y)
#define EMU_ADD_BACKLOG_BATCH_SIZE	64

/**
 * Add backlog from src to dst for flow.
 */
static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount) {
	uint32_t i, n_pkts;
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);
	assert(flow < FLOWS_PER_NODE);

	/* create and enqueue a packet for each MTU, do this in batches */
	struct emu_packet *pkt_ptrs[EMU_ADD_BACKLOG_BATCH_SIZE];
	while (amount > 0) {
		n_pkts = 0;
		for (i = 0; i < MIN(amount, EMU_ADD_BACKLOG_BATCH_SIZE); i++) {
			pkt_ptrs[n_pkts] = create_packet(src, dst, flow);
			n_pkts++;
		}

		/* enqueue the packets to the correct endpoint group packet queue */
		// TODO: support multiple endpoint groups
		while (fp_ring_enqueue_bulk(state->q_epg_new_pkts[0],
				(void **) &pkt_ptrs[0], n_pkts) == -ENOBUFS) {
			/* no space in ring. log and retry. */
			adm_log_emu_endpoint_enqueue_backlog_failed(&state->stat, n_pkts);
		}

		amount -= n_pkts;
	}
}

#endif /* EMULATION_IMPL_H_ */
