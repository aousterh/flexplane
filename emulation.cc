/*
 * emulation.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "api.h"
#include "api_impl.h"
#include "admitted.h"
#include "endpoint_group.h"
#include "drop_tail.h"
#include "router.h"
#include "../protocol/topology.h"

#include <assert.h>

struct emu_state *g_state; /* global emulation state */

void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount) {
	uint32_t i;
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);
	assert(flow < FLOWS_PER_NODE);

	/* create and enqueue a packet for each MTU */
	struct emu_packet *packet;
	for (i = 0; i < amount; i++) {
		packet = create_packet(src, dst, flow);
		if (packet == NULL)
			drop_demand(src, dst, flow);

		/* enqueue the packet to the queue of packets for endpoints */
		// TODO: support multiple endpoint groups
		if (fp_ring_enqueue(state->q_new_packets[0], packet) == -ENOBUFS) {
			/* no space to enqueue this packet, drop it */
			adm_log_emu_endpoint_enqueue_backlog_failed(&g_state->stat);
			drop_packet(packet);
		}
	}
}

void emu_emulate(struct emu_state *state) {
	uint32_t i;
	EndpointGroup *epg;
	Router *router;
	struct emu_packet *packet;

	/* handle push at each endpoint group */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++) {
		epg = state->endpoint_groups[i];
		epg->push();
	}

	/* emulate one timeslot at each router */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		router = state->routers[i];

		/* for each output, try to fetch a packet and send it */
		for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
			router->pull(i, &packet);

			if (packet == NULL)
				continue;

			// TODO: use bulk enqueue?
			// TODO: handle multiple endpoint queues
			epg = state->endpoint_groups[0];
			epg->enqueue_packet_from_network(packet);
		}

		/* push a batch of packets from the network into the router */
		router->push_batch();
	}

	/* handle pull/new packets at each endpoint group */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++) {
		epg = state->endpoint_groups[i];
		epg->pull();
		epg->new_packets();
	}

	/* send out the admitted traffic */
	while (fp_ring_enqueue(state->q_admitted_out, state->admitted) != 0)
		adm_log_emu_wait_for_admitted_enqueue(&state->stat);

	/* get 1 new admitted traffic for the core, init it */
	while (fp_mempool_get(state->admitted_traffic_mempool,
				(void **) &state->admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(&state->stat);
	admitted_init(state->admitted);
}

void emu_cleanup(struct emu_state *state) {
	uint32_t i;
	struct emu_admitted_traffic *admitted;

	/* free all endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++) {
		delete state->endpoint_groups[i];
	}

	/* free all routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		// TODO: call fp_free?
		delete state->routers[i];
	}

	/* return admitted struct to mempool */
	if (state->admitted != NULL)
		fp_mempool_put(state->admitted_traffic_mempool, state->admitted);

	/* empty queue of admitted traffic, return structs to the mempool */
	while (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) == 0)
		fp_mempool_put(state->admitted_traffic_mempool, admitted);
	fp_free(state->q_admitted_out);

	fp_free(state->admitted_traffic_mempool);
	fp_free(state->packet_mempool);
}

void emu_reset_sender(struct emu_state *state, uint16_t src) {

	/* TODO: clear the packets in the routers too? */
	state->endpoint_groups[0]->reset(src);
}

void emu_init_state(struct emu_state *state,
		    struct fp_mempool *admitted_traffic_mempool,
		    struct fp_ring *q_admitted_out,
		    struct fp_mempool *packet_mempool,
		    struct fp_ring **packet_queues, void *args) {
	uint32_t i, pq;
	uint32_t size;
	struct fp_ring *q_to_router;
	struct fp_ring *q_to_endpoints;

	g_state = state;

	pq = 0;
	state->admitted_traffic_mempool = admitted_traffic_mempool;
	state->q_admitted_out = q_admitted_out;
	state->packet_mempool = packet_mempool;

	/* construct topology: 1 router with 1 rack of endpoints */

	/* initialize all the routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		// TODO: use fp_malloc?
		q_to_router = packet_queues[pq++];
		q_to_endpoints = packet_queues[pq++];
		state->routers[i] = new DropTailRouter(i, q_to_router,
				(struct drop_tail_args *) args);
		assert(state->routers[i] != NULL);
	}

	/* initialize all the endpoints in one endpoint group */
	state->q_new_packets[0] = packet_queues[pq++];
	state->endpoint_groups[0] = new DropTailEndpointGroup(EMU_NUM_ENDPOINTS,
			state->q_new_packets[0], q_to_endpoints, q_to_router);
	state->endpoint_groups[0]->init(0, (struct drop_tail_args *) args);
	assert(state->endpoint_groups[0] != NULL);

	/* get 1 admitted traffic for the core, init it */
	while (fp_mempool_get(state->admitted_traffic_mempool,
				(void **) &state->admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(&state->stat);
	admitted_init(state->admitted);
}

struct emu_state *emu_create_state(struct fp_mempool *admitted_traffic_mempool,
				   struct fp_ring *q_admitted_out,
				   struct fp_mempool *packet_mempool,
				   struct fp_ring **packet_queues, void *args) {
	struct emu_state *state = (struct emu_state *) fp_malloc("emu_state",
					    sizeof(struct emu_state));
	if (state == NULL)
		return NULL;

	emu_init_state(state, admitted_traffic_mempool, q_admitted_out,
			packet_mempool, packet_queues, args);

	return state;
}
