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
#include "admissible_log.h"
#include "../graph-algo/platform.h"
#include "assert.h"

int endpoint_init(struct emu_endpoint *ep, uint16_t id, struct emu_ops *ops) {
	assert(ep != NULL);
	assert(ops != NULL);

	ep->id = id;
	ep->ops = &ops->ep_ops;

	return ep->ops->init(ep, ops->args);
}

void endpoint_reset(struct emu_endpoint *ep) {
	struct emu_packet *packet;
	struct fp_mempool *packet_mempool;

	ep->ops->reset(ep);
}

void endpoint_cleanup(struct emu_endpoint *ep) {
	struct emu_packet *packet;

	ep->ops->cleanup(ep);
};

void endpoints_emulate(struct emu_state *state) {
	uint32_t i;
	struct emu_endpoint *ep;
	struct emu_packet *packet;

	// TODO: do in random order
	/* dequeue one packet from each endpoint, send to next hop in network */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		ep = state->endpoints[i];
		ep->ops->send_to_net(ep, &packet);

		if (packet == NULL)
			continue;

		/* TODO: add routing to support enqueuing to different router */
		if (fp_ring_enqueue(state->routers[0]->q_ingress, packet) == -ENOBUFS) {
			adm_log_emu_send_packet_failed(&g_state->stat);
			drop_packet(packet);
		} else {
			adm_log_emu_endpoint_sent_packet(&g_state->stat);
		}
	}

	/* TODO: use burst dequeue */
	/* dequeue packets from network, pass to endpoints */
	while (fp_ring_dequeue(state->q_to_endpoints, (void **) &packet) == 0) {
		ep = state->endpoints[packet->dst];
		ep->ops->rcv_from_net(ep, packet);
	}

	/* dequeue all packets from endpoint apps, give to endpoints */
	/* TODO: use burst dequeue */
	while (fp_ring_dequeue(state->q_from_app, (void **) &packet) == 0) {
		ep = state->endpoints[packet->src];
		ep->ops->rcv_from_app(ep, packet);
	}
}
