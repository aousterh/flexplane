/*
 * emulation.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "api.h"
#include "api_impl.h"
#include "admitted.h"
#include "port.h"

#include <assert.h>

struct emu_state *g_state; /* global emulation state */

void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
					 uint32_t amount) {
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);

	endpoint_add_backlog(state->endpoints[src], dst, amount);
}

void emu_emulate(struct emu_state *state) {
	uint32_t i;
	struct emu_packet *packet;

	/* emulate one timeslot at each endpoint */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		endpoint_emulate(state->endpoints[i]);
	}

	/* emulate one timeslot at each router */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		router_emulate(state->routers[i]);
	}

	/* process all traffic that has arrived at endpoint ports */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		/* dequeue all packets at this port, add them to admitted traffic,
		 * and free them */
		while (fp_ring_dequeue(state->endpoints[i]->q_ingress,
				       (void **) &packet) == 0) {
			admitted_insert_admitted_edge(state->admitted, packet->src,
					packet->dst);
			adm_log_emu_admitted_packet(&state->stat);

			free_packet(packet);
		}
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
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		endpoint_cleanup(state->endpoints[i]);
		fp_free(state->endpoints[i]);
	}

	/* free all routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		router_cleanup(state->routers[i]);
		fp_free(state->routers[i]);
	}

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
	endpoint_reset(state->endpoints[src]);
}

void emu_init_state(struct emu_state *state,
		    struct fp_mempool *admitted_traffic_mempool,
		    struct fp_ring *q_admitted_out,
		    struct fp_mempool *packet_mempool,
		    struct fp_ring **packet_queues,
		    struct emu_ops *ops) {
	uint32_t i, pq;
	uint32_t size;
	struct emu_router *router;
	struct emu_port *port_at_router;

	g_state = state;

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
		endpoint_init(state->endpoints[i], i, packet_queues[pq],
				packet_queues[pq + 1], ops);
		pq += 2;
	}

	/* initialize all the routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		size = EMU_ALIGN(sizeof(struct emu_router)) + ops->rtr_ops.priv_size;
		state->routers[i] = fp_malloc("emu_router", size);
		assert(state->routers[i] != NULL);
		router_init(state->routers[i], i, ops);
	}

	/* initialize all the ports */
	for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
		router = state->routers[i / EMU_ROUTER_NUM_PORTS];
		port_at_router = &router->ports[i % EMU_ROUTER_NUM_PORTS];
		port_pair_init(port_at_router, &state->endpoints[i]->port,
			       packet_queues[pq], packet_queues[pq + 1]);
		pq += 2;
	}

	/* get 1 admitted traffic for the core, init it */
	while (fp_mempool_get(state->admitted_traffic_mempool,
				(void **) &state->admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(&state->stat);
	admitted_init(state->admitted);
}

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
