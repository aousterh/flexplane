/*
 * api_impl.h
 *
 * The implementations of static inline functions declared in api.h
 *
 *  Created on: September 3, 2014
 *      Author: aousterh
 */

#include "api.h"
#include "emulation.h"
#include "endpoint.h"
#include "packet.h"
#include "router.h"
#include "../graph-algo/fp_ring.h"

#include <assert.h>

static inline
struct emu_port *router_port(struct emu_router *rtr, uint32_t port_ind) {
	return &rtr->ports[port_ind];
}

static inline
struct emu_port *endpoint_port(struct emu_endpoint *ep) {
	return &ep->port;
}

static inline
void send_packet(struct emu_port *port, struct emu_packet *packet) {
	if (fp_ring_enqueue(port->q_egress, packet) == -ENOBUFS) {
		adm_log_emu_send_packet_failed(&g_state->stat);
		drop_packet(packet);
	}
}

static inline
void drop_packet(struct emu_packet *packet) {
	drop_demand(packet->src, packet->dst);

	free_packet(packet);

	adm_log_emu_dropped_packet(&g_state->stat);
}

static inline
struct emu_packet *receive_packet(struct emu_port *port) {
	struct emu_packet *packet;

	if (fp_ring_dequeue(port->q_ingress, (void **) &packet) != 0)
		return NULL;

	return packet;
}

static inline
struct emu_packet *dequeue_packet_at_endpoint(struct emu_endpoint *ep) {
	struct emu_packet *packet;

	if (fp_ring_dequeue(ep->q_egress, (void **) &packet) != 0)
		return NULL;

	return packet;
}

static inline
void enqueue_packet_at_endpoint(struct emu_endpoint *ep,
		struct emu_packet *packet) {
	assert(packet->dst == ep->id);

	if (fp_ring_enqueue(ep->q_ingress, packet) == -ENOBUFS) {
		adm_log_emu_endpoint_enqueue_received_failed(&g_state->stat);
		drop_packet(packet);
	}
}

static inline
void free_packet(struct emu_packet *packet) {
	/* return the packet to the mempool */
	fp_mempool_put(g_state->packet_mempool, packet);
}

static inline
uint32_t router_id(struct emu_router *rtr) {
	return rtr->id;
}

static inline
uint32_t endpoint_id(struct emu_endpoint *ep) {
	return ep->id;
}

static inline
void *router_priv(struct emu_router *rtr) {
	return (char *) rtr + EMU_ALIGN(sizeof(struct emu_router));
}

static inline
void *endpoint_priv(struct emu_endpoint *ep) {
	return (char *) ep + EMU_ALIGN(sizeof(struct emu_endpoint));
}

static inline
void *packet_priv(struct emu_packet *packet) {
	return (char *) packet + EMU_ALIGN(sizeof(struct emu_packet));
}
