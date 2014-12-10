/*
 * router.c
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "api.h"
#include "api_impl.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "assert.h"

int router_init(struct emu_router *rtr, uint16_t id, struct emu_ops *ops,
		struct fp_ring *q_ingress) {
	uint16_t i;

	assert(rtr != NULL);
	assert(q_ingress != NULL);
	assert(ops != NULL);

	rtr->id = id;
	rtr->q_ingress = q_ingress;
	rtr->ops = &ops->rtr_ops;

	return rtr->ops->init(rtr, ops->args);
}

void router_cleanup(struct emu_router *rtr) {
	uint16_t i;
	struct emu_packet *packet;

	rtr->ops->cleanup(rtr);

	/* free ingress queue */
	while (fp_ring_dequeue(rtr->q_ingress, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(rtr->q_ingress);
}

void router_emulate(struct emu_router *rtr) {
	uint16_t i;
	struct emu_packet *packet;

	/* for each output, try to fetch a packet and send it */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		rtr->ops->send(rtr, i, &packet);

		if (packet == NULL)
			continue;

		// TODO: use burst enqueue
		// TODO: handle multiple endpoint queues (one per core)
		if (fp_ring_enqueue(g_state->q_to_endpoints, packet) == -ENOBUFS) {
			adm_log_emu_send_packet_failed(&g_state->stat);
			drop_packet(packet);
		} else {
			adm_log_emu_router_sent_packet(&g_state->stat);
		}
	}

	/* pass all incoming packets to the router */
	// TODO: use burst dequeue
	while (fp_ring_dequeue(rtr->q_ingress, (void **) &packet) == 0) {
		rtr->ops->receive(rtr, packet);
	}
}
