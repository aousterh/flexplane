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

        /* create and enqueue a packet for each MTU */
        uint32_t id = start_id;
        struct emu_packet *packet;
        struct fp_ring *packet_queue = state->endpoints[src].q_in;
        while (id != start_id + amount) {
                /* create a packet */
                fp_mempool_get(state->packet_mempool, (void **) &packet);
                packet->src = src;
                packet->dst = dst;
                packet->id = id;

                /* enqueue the packet to the endpoint queue */
                while (fp_ring_enqueue(packet_queue, packet) == -ENOBUFS)
                        printf("error: failed enqueue at endpoint %d\n", src);

                id++;
        }
}

/**
 * Emulate one timeslot at a given endpoint.
 */
static inline
void emu_timeslot_at_endpoint(struct emu_endpoint *endpoint) {
        struct emu_packet *packet;
        struct emu_router *router;

        /* try to dequeue one packet - return if there are none */
        if (fp_ring_dequeue(endpoint->q_in, (void **) &packet) != 0)
                return;

        /* try to enqueue the packet to the next router */
        router = endpoint->router;
        while (fp_ring_enqueue(router->q_in, packet) == -ENOBUFS)
                printf("error: failed enqueue at router\n");
}

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
static inline
void emu_timeslot_at_router(struct emu_state *state, struct emu_router *router) {
        struct emu_packet *packet;
        struct emu_endpoint_output *output;

        /* try to output one packet per output port */
        for (output = &router->endpoint_outputs[0];
             output < &router->endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
             output++) {
                /* try to dequeue one packet for this port */
                if (fp_ring_dequeue(output->q_out, (void **) &packet) != 0)
                        continue;

                /* this packet made it to the endpoint; enqueue as completed */
                while (fp_ring_enqueue(state->finished_packet_q, packet)
                       == -ENOBUFS)
                        printf("error: failed enqueue to finished packet q\n");
        }

        /* move packets from main input queue to individual output queues
           these are packets that arrived at the router during this timeslot */
        while (fp_ring_dequeue(router->q_in, (void **) &packet) == 0) {
                /* assume this is a router with only downward-facing links to
                   endpoints */
                output = &router->endpoint_outputs[packet->dst];
                while (fp_ring_enqueue(output->q_out, packet) == -ENOBUFS)
                        printf("error: failed enqueue within router\n");
        }
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
                emu_timeslot_at_endpoint(endpoint);
        }

        /* emulate one timeslot at each router */
        for (router = &state->routers[0];
             router < &state->routers[EMU_NUM_ROUTERS]; router++) {
                emu_timeslot_at_router(state, router);
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
 * Reset the state of a single endpoint.
 */
static inline
void reset_endpoint_state(struct emu_state *state,
                          struct emu_endpoint *endpoint) {
        struct emu_packet *packet;

        /* return all queued packets to the mempool */
        while (fp_ring_dequeue(endpoint->q_in, (void **) &packet) == 0) {
                /* return queued packets to the mempool */
                fp_mempool_put(state->packet_mempool, packet);
        }
}

/**
 * Reset the state of a single router.
 */
static inline
void reset_router_state(struct emu_state *state, struct emu_router *router) {
        struct emu_packet *packet;
        struct emu_endpoint_output *output;

        /* free packets in the input queue */
        while (fp_ring_dequeue(router->q_in, (void **) &packet) == 0) {
                fp_mempool_put(state->packet_mempool, packet);
        }

        /* free packets in the output queues */
        for (output = &router->endpoint_outputs[0];
             output < &router->endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
             output++) {
                /* try to dequeue one packet for this port */
                if (fp_ring_dequeue(output->q_out, (void **) &packet) == 0) {
                        fp_mempool_put(state->packet_mempool, packet);
                }
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
                reset_endpoint_state(state, endpoint);
        }

        /* reset all routers */
        for (router = &state->routers[0];
             router < &state->routers[EMU_NUM_ROUTERS]; router++) {
                reset_router_state(state, router);
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
