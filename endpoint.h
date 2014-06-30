/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "router.h"
#include "../graph-algo/fp_ring.h"

#include <stdio.h>

/**
 * A representation of an endpoint (server) in the emulated network.
 */
struct emu_endpoint {
        struct fp_ring *q_in;
        struct emu_router *router;
};

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

#endif /* ENDPOINT_H_ */
