/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "packet.h"
#include "router.h"
#include "../graph-algo/fp_ring.h"

#include <stdio.h>

/**
 * A representation of an endpoint (server) in the emulated network.
 */
struct emu_endpoint {
        uint16_t id;
        struct fp_ring *q_in;
        struct emu_router *router;
};

/**
 * Add backlog to dst at this endpoint.
 */
static inline
void endpoint_add_backlog(struct emu_endpoint *endpoint,
                          struct fp_mempool *packet_mempool, uint16_t dst,
                          uint32_t amount, uint16_t start_id) {

        /* create and enqueue a packet for each MTU */
        uint32_t id = start_id;
        struct emu_packet *packet;
        while (id != start_id + amount) {
                /* create a packet */
                fp_mempool_get(packet_mempool, (void **) &packet);
                packet_init(packet, endpoint->id, dst, id);

                /* enqueue the packet to the endpoint queue */
                while (fp_ring_enqueue(endpoint->q_in, packet) == -ENOBUFS)
                        printf("error: failed enqueue at endpoint %d\n",
                               endpoint->id);

                id++;
        }
}

/**
 * Emulate one timeslot at a given endpoint.
 */
static inline
void endpoint_emulate_timeslot(struct emu_endpoint *endpoint) {
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
 * Reset the state of a single endpoint.
 */
static inline
void endpoint_reset_state(struct emu_endpoint *endpoint,
                          struct fp_mempool * packet_mempool) {
        struct emu_packet *packet;

        /* return all queued packets to the mempool */
        while (fp_ring_dequeue(endpoint->q_in, (void **) &packet) == 0) {
                /* return queued packets to the mempool */
                fp_mempool_put(packet_mempool, packet);
        }
}

/**
 * Initialize the state of a single endpoint.
 */
static inline
void endpoint_init_state(struct emu_endpoint *endpoint, uint16_t id,
                         struct fp_ring *q_in, struct emu_router *router) {
        endpoint->id = id;
        endpoint->q_in = q_in;
        endpoint->router = router;
}

#endif /* ENDPOINT_H_ */
