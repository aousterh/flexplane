/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "packet.h"
#include "topology.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

struct emu_endpoint;
struct emu_router;

/**
 * An output queue to an endpoint in the emulated network.
 */
struct emu_endpoint_output {
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
static inline
void router_emulate_timeslot(struct emu_router *router,
                             struct fp_ring *finished_packet_q) {
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
                while (fp_ring_enqueue(finished_packet_q, packet)
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
                /* try to dequeue one packet for this port */
                if (fp_ring_dequeue(output->q_out, (void **) &packet) == 0) {
                        fp_mempool_put(packet_mempool, packet);
                }
        }
}


#endif /* ROUTER_H_ */
