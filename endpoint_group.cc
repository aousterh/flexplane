/*
 * endpoint_group.cc
 *
 *  Created on: December 11, 2014
 *      Author: aousterh
 */

#include "endpoint_group.h"
#include "emulation.h"
#include "endpoint.h"
#include "api.h"
#include "api_impl.h"
#include "packet.h"
#include "admissible_log.h"
#include "../graph-algo/platform.h"
#include "../graph-algo/fp_ring.h"
#include "assert.h"

#define ENDPOINT_MAX_BURST	(EMU_NUM_ENDPOINTS * 2)

EndpointGroup::EndpointGroup(uint16_t num_endpoints,
		struct fp_ring *q_new_packets, struct fp_ring *q_from_network,
		struct fp_ring *q_to_router) {
	this->num_endpoints = num_endpoints;
	this->q_new_packets = q_new_packets;
	this->q_from_network = q_from_network;
	this->q_to_router = q_to_router;
}

EndpointGroup::~EndpointGroup() {
	uint16_t i;
	struct emu_packet *p;

	for (i = 0; i < num_endpoints; i++)
		delete endpoints[i];

	/* free q_new_packets and return its packets to the mempool */
	while (fp_ring_dequeue(q_new_packets, (void **) &p) == 0) {
		free_packet(p);
	}
	fp_free(q_new_packets);

	/* free q_from_network and return its packets to the mempool */
	while (fp_ring_dequeue(q_from_network, (void **) &p) == 0) {
		free_packet(p);
	}
	fp_free(q_from_network);

	/* router will free its q_to_router */
}

void EndpointGroup::init(uint16_t start_id, struct drop_tail_args *args) {
	uint16_t i;

	/* initialize all the endpoints */
	for (i = 0; i < num_endpoints; i++) {
		endpoints[i] = make_endpoint(start_id + i, args);
		assert(endpoints[i] != NULL);
	}
}

void EndpointGroup::reset(uint16_t endpoint_id) {
	endpoints[endpoint_id]->reset();
}

void EndpointGroup::new_packets() {
	uint16_t num_packets, i;
	struct emu_packet *packets[ENDPOINT_MAX_BURST];
	Endpoint *ep;

	/* dequeue packets from network, pass to endpoints */
	num_packets = fp_ring_dequeue_burst(q_new_packets, (void **) &packets,
			ENDPOINT_MAX_BURST);
	for (i = 0; i < num_packets; i++) {
		// TODO: support multiple endpoint groups
		ep = endpoints[packets[i]->src];
		ep->new_packet(packets[i]);
	}
}

void EndpointGroup::push() {
	uint16_t num_packets, i;
	struct emu_packet *packets[ENDPOINT_MAX_BURST];
	Endpoint *ep;

	/* dequeue packets from network, pass to endpoints */
	num_packets = fp_ring_dequeue_burst(q_from_network, (void **) &packets,
			ENDPOINT_MAX_BURST);
	for (i = 0; i < num_packets; i++) {
		// TODO: support multiple endpoint groups
		ep = endpoints[packets[i]->dst];
		ep->push(packets[i]);
	}
}

void EndpointGroup::pull() {
	uint16_t i;
	Endpoint *ep;
	struct emu_packet *packet;

	// TODO: do in random order
	/* dequeue one packet from each endpoint, send to next hop in network */
	for (i = 0; i < num_endpoints; i++) {
		ep = endpoints[i];
		ep->pull(&packet);

		if (packet == NULL)
			continue;

		// TODO: use bulk enqueue?
		if (fp_ring_enqueue(q_to_router, packet) == -ENOBUFS) {
			adm_log_emu_send_packet_failed(&g_state->stat);
			drop_packet(packet);
		} else {
			adm_log_emu_endpoint_sent_packet(&g_state->stat);
		}
	}
}

void EndpointGroup::enqueue_packet_from_network(struct emu_packet *packet) {
	if (fp_ring_enqueue(q_from_network, packet) == -ENOBUFS) {
		adm_log_emu_send_packet_failed(&g_state->stat);
		drop_packet(packet);
	} else {
		adm_log_emu_router_sent_packet(&g_state->stat);
	}
}
