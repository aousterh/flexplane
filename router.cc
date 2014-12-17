/*
 * router.cc
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

void Router::push_batch() {
	uint16_t i, num_packets;
	struct emu_packet *packets[ROUTER_MAX_BURST];

	/* pass all incoming packets to the router */
	num_packets = fp_ring_dequeue_burst(this->q_ingress,
			(void **) &packets, ROUTER_MAX_BURST);
#ifdef EMU_NO_BATCH_CALLS
	for (i = 0; i < num_packets; i++) {
		this->push(packets[i]);
	}
#else
	this->push_batch(&packets[0], num_packets);
#endif
}
