/*
 * emulation.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

/**
 * Add backlog from src to dst
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
                     uint32_t amount, uint16_t start_id) {
        endpoint_add_backlog(&state->endpoints[src], state->packet_mempool, dst,
                             amount, start_id);
}

/**
 * Emulate a single timeslot.
 */
void emu_timeslot(struct emu_state *state) {
        struct emu_endpoint *endpoint;
        struct emu_router *router;
        struct emu_packet *packet;

        /* emulate one timeslot at each endpoint */
        for (endpoint = &state->endpoints[0];
             endpoint < &state->endpoints[EMU_NUM_ENDPOINTS]; endpoint++) {
                endpoint_emulate_timeslot(endpoint);
        }

        /* emulate one timeslot at each router */
        for (router = &state->routers[0];
             router < &state->routers[EMU_NUM_ROUTERS]; router++) {
                router_emulate_timeslot(router, state->finished_packet_q);
        }

        /* process all admitted traffic */
        printf("admitted traffic:\n");
        while (fp_ring_dequeue(state->finished_packet_q, (void **) &packet)
               == 0) {
                printf("src %d to dst %d (%d)\n", packet->src, packet->dst,
                       packet->id);
                fp_mempool_put(state->packet_mempool, packet);
        }
}

/**
 * Reset the emulation state (clear all demands, packets, etc.).
 */
void emu_reset_state(struct emu_state *state) {
        struct emu_endpoint *endpoint;
        struct emu_router *router;
        struct emu_packet *packet;

        /* reset all endpoints */
        for (endpoint = &state->endpoints[0];
             endpoint < &state->endpoints[EMU_NUM_ENDPOINTS]; endpoint++) {
                endpoint_reset_state(endpoint, state->packet_mempool);
        }

        /* reset all routers */
        for (router = &state->routers[0];
             router < &state->routers[EMU_NUM_ROUTERS]; router++) {
                router_reset_state(router, state->packet_mempool);
        }

        /* empty queue of finished packets, return them to the mempool */
        while (fp_ring_dequeue(state->finished_packet_q, (void **) &packet)
               == 0) {
                fp_mempool_put(state->packet_mempool, packet);
        }
}

/**
 * Initialize an emulation state.
 */
void init_state(struct emu_state *state, struct fp_mempool *packet_mempool,
                struct fp_ring **packet_queues) {
        uint16_t i, pq;
        struct emu_endpoint *endpoint;
        struct emu_router *router;

        pq = 0;
        state->packet_mempool = packet_mempool;
        state->finished_packet_q = packet_queues[pq++];

        /* construct topology: 1 router with 1 rack of endpoints */
        for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
                endpoint = &state->endpoints[i];
                endpoint->id = i;
                endpoint->q_in = packet_queues[pq++];
                endpoint->router = &state->routers[0];

                state->routers[0].endpoint_outputs[i].q_out = packet_queues[pq++];
        }

        for (i = 0; i < EMU_NUM_ROUTERS; i++) {
                router = &state->routers[i];
                router->q_in = packet_queues[pq++];
        }
}

/**
 * Returns an initialized emulation state, or NULL on error.
 */
struct emu_state *emu_create_state(struct fp_mempool *packet_mempool,
                                   struct fp_ring **packet_queues) {
        struct emu_state *state = fp_malloc("emu_state", sizeof(struct emu_state));
        if (state == NULL)
                return NULL;

        init_state(state, packet_mempool, packet_queues);
        emu_reset_state(state);

        return state;
}
