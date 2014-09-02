/*
 * endpoint.c
 *
 *  Created on: July 16, 2014
 *      Author: aousterh
 */

#include "endpoint.h"

#include "api.h"
#include "packet.h"
#include "emulation.h"
#include "../graph-algo/admissible_algo_log.h"
#include "../graph-algo/platform.h"
#include "assert.h"

/**
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

/**
 * Returns a pointer to the port at endpoint ep.
 */
struct emu_port *endpoint_port(struct emu_endpoint *ep) {
	return ep->port;
}

/**
 * Returns the id of endpoint ep.
 */
uint32_t endpoint_id(struct emu_endpoint *ep) {
	return ep->id;
}

/**
 * Dequeues a packet at an endpoint from the network stack. Returns a pointer
 * to the packet, or NULL if none are available.
 */
struct emu_packet *dequeue_packet_at_endpoint(struct emu_endpoint *ep) {
	struct emu_packet *packet;

	if (fp_ring_dequeue(ep->q_egress, (void **) &packet) != 0)
		return NULL;

	return packet;
}


/**
 * Functions declared in endpoint.h, used only within the framework
 */

/**
 * Initialize an endpoint.
 * @return 0 on success, negative value on error.
 */
int endpoint_init(struct emu_endpoint *ep, uint16_t id,
				  struct fp_ring *q_egress, struct emu_port *port,
				  struct emu_state *state, struct emu_ops *ops) {
	assert(ep != NULL);
	assert(q_egress != NULL);
	assert(port != NULL);
	assert(state != NULL);
	assert(ops != NULL);

	ep->id = id;
	ep->q_egress = q_egress;
	ep->port = port;
	ep->state = state;
	ep->ops = &ops->ep_ops;

	return ep->ops->init(ep, ops->args);
}

/**
 * Reset an endpoint. This happens when endpoints lose sync with the
 * arbiter. To resync, a reset occurs, then backlogs are re-added based
 * on endpoint reports.
 */
void endpoint_reset(struct emu_endpoint *ep) {
	struct emu_packet *packet;
	struct fp_mempool *packet_mempool;

	ep->ops->reset(ep);

	/* return all queued packets to the mempool */
	packet_mempool = ep->state->packet_mempool;
	while (fp_ring_dequeue(ep->q_egress, (void **) &packet) == 0) {
		fp_mempool_put(packet_mempool, packet);
	}
}

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void endpoint_cleanup(struct emu_endpoint *ep) {
	struct emu_packet *packet;

	ep->ops->cleanup(ep);

	/* free all currently queued packets */
	while (fp_ring_dequeue(ep->q_egress, (void **) &packet) == 0) {
		free_packet(packet);
	}
};

/**
 * Emulate one timeslot at a given endpoint.
 */
void endpoint_emulate(struct emu_endpoint *ep) {
	ep->ops->emulate(ep);
}

/**
 * Add backlog to dst at this endpoint.
 */
void endpoint_add_backlog(struct emu_endpoint *ep, uint16_t dst,
						  uint32_t amount) {
	uint32_t i;

	/* create and enqueue a packet for each MTU */
	struct emu_packet *packet;
	for (i = 0; i < amount; i++) {
		packet = create_packet(ep->state, ep->id, dst);
		if (packet == NULL)
			drop_demand(ep->state, ep->id, dst);

		/* enqueue the packet to the endpoint queue */
		if (fp_ring_enqueue(ep->q_egress, packet) == -ENOBUFS) {
			/* no space to enqueue this packet at the endpoint, drop it */
			adm_log_emu_wait_for_endpoint_enqueue(&ep->state->stat);
			drop_packet(packet);
		}
	}
}
