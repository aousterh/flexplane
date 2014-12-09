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

	/* free egress queue in port (ingress will be freed by other port) */
	while (fp_ring_dequeue(ep->port.q_egress, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(ep->port.q_egress);
};

void endpoints_emulate(struct emu_state *state) {
	uint32_t i;
	struct emu_endpoint *ep;
	struct emu_packet *packet;
	struct emu_port *port;

	// TODO: do in random order
	/* dequeue one packet from each endpoint, send to next hop in network */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		ep = state->endpoints[i];
		ep->ops->send_to_net(ep, &packet);
		if (packet != NULL) {
			port = endpoint_port(ep); // TODO: get rid of ports
			/* try to transmit the packet to the next router */
			adm_log_emu_endpoint_sent_packet(&g_state->stat);
			send_packet(port, packet);
		}
	}

	/* TODO: replace ports with single queue */
	/* dequeue packets from network, pass to endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		ep = state->endpoints[i];
		port = endpoint_port(ep);
		if ((packet = receive_packet(port)) != NULL)
			ep->ops->rcv_from_net(ep, packet);
	}

	/* dequeue all packets from endpoint apps, give to endpoints */
	while (fp_ring_dequeue(state->q_from_app, (void **) &packet) == 0) {
		ep = state->endpoints[packet->src];
		ep->ops->rcv_from_app(ep, packet);
	}
}
