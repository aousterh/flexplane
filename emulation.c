/*
 * emulation.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"

#include "admitted.h"

#include <assert.h>
#include <stdio.h>

/**
 * Add backlog from src to dst
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
                     uint32_t amount, uint16_t start_id) {
        assert(src < EMU_NUM_ENDPOINTS);
        assert(dst < EMU_NUM_ENDPOINTS);

        endpoint_add_backlog(&state->endpoints[src], state->packet_mempool, dst,
                             amount, start_id);
}

/**
 * Emulate a single timeslot.
 */
void emu_timeslot(struct emu_state *state) {
        struct emu_endpoint *endpoint;
        struct emu_router *router;

        /* emulate one timeslot at each endpoint */
        for (endpoint = &state->endpoints[0];
             endpoint < &state->endpoints[EMU_NUM_ENDPOINTS]; endpoint++) {
                endpoint_emulate_timeslot(endpoint);
        }

        /* emulate one timeslot at each router */
        for (router = &state->routers[0];
             router < &state->routers[EMU_NUM_ROUTERS]; router++) {
                router_emulate_timeslot(router, state);
        }
}

/**
 * Reset the emulation state (clear all demands, packets, etc.).
 */
void emu_reset_state(struct emu_state *state) {
        struct emu_endpoint *endpoint;
        struct emu_router *router;
        struct emu_admitted_traffic *admitted;

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

        /* empty queue of admitted traffic, return structs to the mempool */
        while (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) == 0)
                fp_mempool_put(state->admitted_traffic_mempool, admitted);
}

/**
 * Initialize an emulation state.
 */
void init_state(struct emu_state *state,
                struct fp_mempool *admitted_traffic_mempool,
                struct fp_ring *q_admitted_out,
                struct fp_mempool *packet_mempool,
                struct fp_ring **packet_queues,
                uint16_t router_output_port_capacity) {
        uint16_t i, pq;

        pq = 0;
        state->admitted_traffic_mempool = admitted_traffic_mempool;
        state->q_admitted_out = q_admitted_out;
        state->packet_mempool = packet_mempool;

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
struct emu_state *emu_create_state(struct fp_mempool *admitted_traffic_mempool,
                                   struct fp_ring *q_admitted_out,
                                   struct fp_mempool *packet_mempool,
                                   struct fp_ring **packet_queues,
                                   uint16_t router_output_port_capacity) {
        struct emu_state *state = fp_malloc("emu_state",
                                            sizeof(struct emu_state));
        if (state == NULL)
                return NULL;

        init_state(state, admitted_traffic_mempool, q_admitted_out,
                   packet_mempool, packet_queues,
                   router_output_port_capacity);
        emu_reset_state(state);

        return state;
}
