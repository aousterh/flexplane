/*
 * endpoint.c
 *
 *  Created on: July 16, 2014
 *      Author: aousterh
 */

#include "endpoint.h"

#include "api.h"
#include "api_impl.h"
#include "packet.h"
#include "emulation.h"
#include "../graph-algo/admissible_algo_log.h"
#include "../graph-algo/platform.h"
#include "assert.h"

int endpoint_init(struct emu_endpoint *ep, uint16_t id,
				  struct fp_ring *q_egress, struct emu_ops *ops) {
	assert(ep != NULL);
	assert(q_egress != NULL);
	assert(ops != NULL);

	ep->id = id;
	ep->q_egress = q_egress;
	ep->ops = &ops->ep_ops;

	return ep->ops->init(ep, ops->args);
}

void endpoint_reset(struct emu_endpoint *ep) {
	struct emu_packet *packet;
	struct fp_mempool *packet_mempool;

	ep->ops->reset(ep);

	/* return all queued packets to the mempool */
	packet_mempool = g_state->packet_mempool;
	while (fp_ring_dequeue(ep->q_egress, (void **) &packet) == 0) {
		fp_mempool_put(packet_mempool, packet);
	}
}

void endpoint_cleanup(struct emu_endpoint *ep) {
	struct emu_packet *packet;

	ep->ops->cleanup(ep);

	/* free queue of packets from the endpoint application */
	while (fp_ring_dequeue(ep->q_egress, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(ep->q_egress);

	/* free egress queue in port (ingress will be freed by other port) */
	while (fp_ring_dequeue(ep->port.q_egress, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(ep->port.q_egress);
};

void endpoint_emulate(struct emu_endpoint *ep) {
	ep->ops->emulate(ep);
}

void endpoint_add_backlog(struct emu_endpoint *ep, uint16_t dst,
						  uint32_t amount) {
	uint32_t i;

	/* create and enqueue a packet for each MTU */
	struct emu_packet *packet;
	for (i = 0; i < amount; i++) {
		packet = create_packet(ep->id, dst);
		if (packet == NULL)
			drop_demand(ep->id, dst);

		/* enqueue the packet to the endpoint queue */
		if (fp_ring_enqueue(ep->q_egress, packet) == -ENOBUFS) {
			/* no space to enqueue this packet at the endpoint, drop it */
			adm_log_emu_wait_for_endpoint_enqueue(&g_state->stat);
			drop_packet(packet);
		}
	}
}
