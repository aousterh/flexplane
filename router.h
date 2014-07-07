/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "config.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

struct emu_endpoint;
struct emu_router;
struct emu_state;

/**
 * An output queue to an endpoint in the emulated network.
 */
struct emu_endpoint_output {
        uint16_t capacity; /* if statically configured */
        uint16_t count; /* current utilization */
        struct fp_ring *q_out;
};

/**
 * An output queue to a router in the emulated network.
 */
struct emu_router_output {
        struct fp_ring *q_out;
        struct emu_router *router;
};

/**
 * A representation of a router in the emulated network. For now, there is a
 * fixed capacity for each output queue.
 */
struct emu_router {
        struct fp_ring *q_in;
        struct emu_endpoint_output endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
        struct emu_router_output router_outputs[EMU_ROUTER_MAX_ROUTER_PORTS];
};

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void router_emulate_timeslot(struct emu_router *router,
                             struct emu_state *state);

/**
 * Reset the state of a single router.
 */
static inline
void router_reset_state(struct emu_router *router,
                        struct fp_mempool *packet_mempool) {
        struct emu_packet *packet;
        struct emu_endpoint_output *output;

        /* free packets in the input queue */
        while (fp_ring_dequeue(router->q_in, (void **) &packet) == 0) {
                fp_mempool_put(packet_mempool, packet);
        }

        /* free packets in the output queues */
        for (output = &router->endpoint_outputs[0];
             output < &router->endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
             output++) {
                /* return all queued packets to the mempool */
                while (fp_ring_dequeue(output->q_out, (void **) &packet) == 0) {
                        fp_mempool_put(packet_mempool, packet);
                }
                output->count = 0;
        }
}

/**
 * Initialize the state of a single router. Return the number of packet queues
 * used.
 */
static inline
uint16_t router_init_state(struct emu_router *router,
                           struct fp_ring **packet_queues,
                           uint16_t output_port_capacity) {
        uint16_t pq;
        struct emu_endpoint_output *output;

        for (pq = 0; pq < EMU_NUM_ENDPOINTS; pq++) {
                output = &router->endpoint_outputs[pq];

                output->capacity = output_port_capacity;
                output->q_out = packet_queues[pq];
        }
        router->q_in = packet_queues[pq++];

        return pq;
}

#endif /* ROUTER_H_ */
