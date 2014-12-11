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

#define ROUTER_MAX_BURST	(EMU_NUM_ENDPOINTS * 2)

Router::Router(uint16_t id, struct fp_ring *q_ingress) {
	assert(q_ingress != NULL);

	this->id = id;
	this->q_ingress = q_ingress;
}

Router::~Router() {
	uint16_t i;
	struct emu_packet *packet;

	/* free ingress queue */
	while (fp_ring_dequeue(this->q_ingress, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(this->q_ingress);
}

void Router::emulate() {
	uint16_t i;
	struct emu_packet *packet;
	uint16_t num_packets;
	struct emu_packet *packets[ROUTER_MAX_BURST];

	/* for each output, try to fetch a packet and send it */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		this->pull(i, &packet);

		if (packet == NULL)
			continue;

		// TODO: use bulk enqueue?
		// TODO: handle multiple endpoint queues (one per core)
		if (fp_ring_enqueue(g_state->q_to_endpoints, packet) == -ENOBUFS) {
			adm_log_emu_send_packet_failed(&g_state->stat);
			drop_packet(packet);
		} else {
			adm_log_emu_router_sent_packet(&g_state->stat);
		}
	}

	/* pass all incoming packets to the router */
	num_packets = fp_ring_dequeue_burst(this->q_ingress,
			(void **) &packets, ROUTER_MAX_BURST);
	for (i = 0; i < num_packets; i++) {
		this->push(packets[i]);
	}
}
