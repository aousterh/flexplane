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

/**
 * Add backlog from src to dst for flow.
 */
static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount) {
	uint32_t i;
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);
	assert(flow < FLOWS_PER_NODE);

	/* create and enqueue a packet for each MTU */
	struct emu_packet *packet;
	for (i = 0; i < amount; i++) {
		packet = create_packet(src, dst, flow);
		if (packet == NULL) {
			drop_demand(src, dst, flow);
			continue;
		}

		/* enqueue the packet to the queue of packets for endpoints */
		// TODO: support multiple endpoint groups
		if (fp_ring_enqueue(state->q_epg_new_pkts[0], packet) == -ENOBUFS) {
			/* no space to enqueue this packet, drop it */
			adm_log_emu_endpoint_enqueue_backlog_failed(&state->stat);
			drop_packet(packet);
		}
	}
}

#endif /* EMULATION_IMPL_H_ */
