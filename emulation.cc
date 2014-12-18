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
#include "drop_tail.h"
#include "endpoint_group.h"
#include "router.h"
#include "../protocol/topology.h"

#include <assert.h>

#define ROUTER_MAX_BURST	(EMU_NUM_ENDPOINTS * 2)

emu_state *g_state; /* global emulation state */

static inline void free_packet_ring(struct fp_ring *packet_ring);

void emu_init_state(struct emu_state *state,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_ring *q_admitted_out, struct fp_mempool *packet_mempool,
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
		state->routers[i] = new DropTailRouter(i,
				(struct drop_tail_args *) args);
		assert(state->routers[i] != NULL);
		state->q_router_ingress[i] = q_to_router;
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

		/* free ingress queue for this router (and all its packets) */
		free_packet_ring(state->q_router_ingress[i]);
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

/*
 * Emulate a timeslot at a single router with index @index
 */
static inline void emu_emulate_router(struct emu_state *state,
		uint32_t index) {
	Router *router;
	EndpointGroup *epg;
	uint16_t i, num_packets;

	/* get the corresponding router */
	router = state->routers[index];

#ifdef EMU_NO_BATCH_CALLS
	/* for each output, try to fetch a packet and send it */
	for (uint32_t j = 0; j < EMU_ROUTER_NUM_PORTS; j++) {
		router->pull(j, &packet);

		if (packet == NULL)
			continue;

		// TODO: handle multiple endpoint queues
		epg = state->endpoint_groups[0];
		epg->enqueue_packet_from_network(packet);
	}
#else
	struct emu_packet *pkt_ptrs[EMU_ROUTER_NUM_PORTS];

	uint32_t n_pkts = router->pull_batch(pkt_ptrs, EMU_ROUTER_NUM_PORTS);

	for (uint32_t j = 0; j < n_pkts; j++) {
		epg = state->endpoint_groups[0];
		epg->enqueue_packet_from_network(pkt_ptrs[j]);
	}
#endif

	/* push a batch of packets from the network into the router */
	struct emu_packet *packets[ROUTER_MAX_BURST];

	/* pass all incoming packets to the router */
	num_packets = fp_ring_dequeue_burst(state->q_router_ingress[index],
			(void **) &packets, ROUTER_MAX_BURST);
#ifdef EMU_NO_BATCH_CALLS
	for (i = 0; i < num_packets; i++) {
		router->push(packets[i]);
	}
#else
	router->push_batch(&packets[0], num_packets);
#endif
}

void emu_emulate(struct emu_state *state) {
	uint32_t i;
	EndpointGroup *epg;
	struct emu_packet *packet;

	/* handle push at each endpoint group */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++) {
		epg = state->endpoint_groups[i];
		epg->push();
	}

	/* emulate one timeslot at each router */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		emu_emulate_router(state, i);
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

void emu_reset_sender(struct emu_state *state, uint16_t src) {

	/* TODO: clear the packets in the routers too? */
	state->endpoint_groups[0]->reset(src);
}

/* frees all the packets in an fp_ring, and frees the ring itself */
static inline void free_packet_ring(struct fp_ring *packet_ring) {
	struct emu_packet *packet;

	while (fp_ring_dequeue(packet_ring, (void **) &packet) == 0) {
		free_packet(packet);
	}
	fp_free(packet_ring);
}
