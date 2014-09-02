/*
 * emulation.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "api.h"
#include "admitted.h"
#include "port.h"

#include <assert.h>

/**
 * Add backlog from src to dst
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
					 uint32_t amount) {
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);

	endpoint_add_backlog(state->endpoints[src], dst, amount);
}

/**
 * Emulate a single timeslot.
 */
void emu_emulate(struct emu_state *state) {
	uint32_t i;
	struct emu_packet *packet;

	/* get 1 admitted traffic for the core, init it */
	while (fp_mempool_get(state->admitted_traffic_mempool,
				(void **) &state->admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(&state->stat);
	admitted_init(state->admitted);

	/* emulate one timeslot at each endpoint */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		endpoint_emulate(state->endpoints[i]);
	}

	/* emulate one timeslot at each router */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		router_emulate(state->routers[i]);
	}

	/* process all traffic that has arrived at endpoint ports */
	for (i = EMU_NUM_ENDPOINTS; i < EMU_NUM_PORTS; i++) {
		/* dequeue all packets at this port, add them to admitted traffic,
		 * and free them */
		while (fp_ring_dequeue(state->ports[i].q, (void **) &packet) == 0) {
			admitted_insert_admitted_edge(state->admitted, packet->src,
					packet->dst);

			free_packet(packet);
		}
	}

	/* send out the admitted traffic */
	while (fp_ring_enqueue(state->q_admitted_out, state->admitted) != 0)
		adm_log_emu_wait_for_admitted_enqueue(&state->stat);
	state->admitted = NULL;
}

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void emu_cleanup(struct emu_state *state) {
	uint32_t i;
	struct emu_admitted_traffic *admitted;

	/* reset all endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		endpoint_cleanup(state->endpoints[i]);
		fp_free(state->endpoints[i]);
	}

	/* reset all routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		router_cleanup(state->routers[i]);
		fp_free(state->routers[i]);
	}

	fp_free(state->admitted_traffic_mempool);

	if (state->admitted != NULL)
		fp_mempool_put(state->admitted_traffic_mempool, state->admitted);

	/* empty queue of admitted traffic, return structs to the mempool */
	while (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) == 0)
		fp_mempool_put(state->admitted_traffic_mempool, admitted);
	fp_free(state->q_admitted_out);

	fp_free(state->packet_mempool);
}

/**
 * Reset the emulation state for a single sender.
 */
void emu_reset_sender(struct emu_state *state, uint16_t src) {

	/* TODO: clear the packets in the routers too? */
	endpoint_reset(state->endpoints[src]);
}

/**
 * Initialize an emulation state.
 */
void emu_init_state(struct emu_state *state,
		    struct fp_mempool *admitted_traffic_mempool,
		    struct fp_ring *q_admitted_out,
		    struct fp_mempool *packet_mempool,
		    struct fp_ring **packet_queues,
		    struct emu_ops *ops) {
	uint32_t i, pq;
	uint32_t size;

	pq = 0;
	state->admitted_traffic_mempool = admitted_traffic_mempool;
	state->q_admitted_out = q_admitted_out;
	state->packet_mempool = packet_mempool;

	/* construct topology: 1 router with 1 rack of endpoints */

	/* initialize all the endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		size = EMU_ALIGN(sizeof(struct emu_endpoint)) + ops->ep_ops.priv_size;
		state->endpoints[i] = fp_malloc("emu_endpoint", size);
		assert(state->endpoints[i] != NULL);
		endpoint_init(state->endpoints[i], i, packet_queues[pq++],
				&state->ports[i], state, ops);
	}

	/* initialize all the routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		size = EMU_ALIGN(sizeof(struct emu_router)) + ops->rtr_ops.priv_size;
		state->routers[i] = fp_malloc("emu_router", size);
		assert(state->routers[i] != NULL);
		router_init(state->routers[i], i, &state->ports[0], state, ops);
	}

	/* initialize all the endpoint ports */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		port_init(&state->ports[i], packet_queues[pq++], NIC_TYPE_ENDPOINT,
				&state->endpoints[i], NIC_TYPE_ROUTER,
				&state->routers[i / EMU_ROUTER_NUM_PORTS]);
	}
	/* initialize all the router ports */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		port_init(&state->ports[EMU_NUM_ENDPOINTS + i], packet_queues[pq++],
				NIC_TYPE_ROUTER, &state->routers[i / EMU_ROUTER_NUM_PORTS],
				NIC_TYPE_ENDPOINT, &state->endpoints[i]);
	}
}

/**
 * Returns an initialized emulation state, or NULL on error.
 */
struct emu_state *emu_create_state(struct fp_mempool *admitted_traffic_mempool,
				   struct fp_ring *q_admitted_out,
				   struct fp_mempool *packet_mempool,
				   struct fp_ring **packet_queues,
				   struct emu_ops *ops) {
	struct emu_state *state = fp_malloc("emu_state",
					    sizeof(struct emu_state));
	if (state == NULL)
		return NULL;

	emu_init_state(state, admitted_traffic_mempool, q_admitted_out,
			packet_mempool, packet_queues, ops);

	return state;
}
