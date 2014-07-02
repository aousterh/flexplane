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
        printf("finished traffic:\n");
        while (fp_ring_dequeue(state->finished_packet_q, (void **) &packet)
               == 0) {
                packet_print(packet);
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
                struct fp_ring **packet_queues,
                uint16_t router_output_port_capacity) {
        uint16_t i, pq;

        pq = 0;
        state->packet_mempool = packet_mempool;
        state->finished_packet_q = packet_queues[pq++];

        /* construct topology: 1 router with 1 rack of endpoints */
        for (i = 0; i < EMU_NUM_ENDPOINTS; i++) {
                endpoint_init_state(&state->endpoints[i], i,
                                    packet_queues[pq++], &state->routers[0]);
        }

        for (i = 0; i < EMU_NUM_ROUTERS; i++) {
                pq += router_init_state(&state->routers[i], packet_queues + pq,
                                        router_output_port_capacity);
        }
}

/**
 * Returns an initialized emulation state, or NULL on error.
 */
struct emu_state *emu_create_state(struct fp_mempool *packet_mempool,
                                   struct fp_ring **packet_queues,
                                   uint16_t router_output_port_capacity) {
        struct emu_state *state = fp_malloc("emu_state", sizeof(struct emu_state));
        if (state == NULL)
                return NULL;

        init_state(state, packet_mempool, packet_queues, router_output_port_capacity);
        emu_reset_state(state);

        return state;
}
