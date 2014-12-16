/*
 * api_impl.h
 *
 * The implementations of static inline functions declared in api.h
 *
 *  Created on: September 3, 2014
 *      Author: aousterh
 */

#ifndef API_IMPL_H__
#define API_IMPL_H__

#include "admitted.h"
#include "api.h"
#include "endpoint.h"
#include "packet.h"
#include "router.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <assert.h>

static inline
void drop_packet(struct emu_packet *packet) {
	drop_demand(packet->src, packet->dst, packet->flow);

	free_packet(packet);

	adm_log_emu_dropped_packet(&g_state->stat);
}

static inline
void enqueue_packet_at_endpoint(struct emu_packet *packet) {
	admitted_insert_admitted_edge(g_state->admitted, packet->src,
			packet->dst, packet->flow);
	adm_log_emu_admitted_packet(&g_state->stat);

	free_packet(packet);
}

static inline
void free_packet(struct emu_packet *packet) {
	/* return the packet to the mempool */
	fp_mempool_put(g_state->packet_mempool, packet);
}

static inline
uint16_t get_output_queue(Router *rtr, struct emu_packet *p) {
	return p->dst;
}

static inline
void *packet_priv(struct emu_packet *packet) {
	return (char *) packet + EMU_ALIGN(sizeof(struct emu_packet));
}

#endif /* API_IMPL_H__ */
