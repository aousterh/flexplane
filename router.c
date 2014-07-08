/*
 * router.c
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#include "router.h"

#include "admitted.h"
#include "emulation.h"
#include "packet.h"

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void router_emulate_timeslot(struct emu_router *router,
                             struct emu_state *state) {
        struct emu_packet *packet;
        struct emu_endpoint_output *output;
        struct emu_admitted_traffic *admitted;

        /* get admitted traffic, init it */
        while (fp_mempool_get(state->admitted_traffic_mempool,
                              (void **) &admitted) != 0)
                printf("error: failed to get admitted traffic\n");
        admitted_init(admitted);

        /* try to output one packet per output port */
        for (output = &router->endpoint_outputs[0];
             output < &router->endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
             output++) {
                /* try to dequeue one packet for this port */
                if (fp_ring_dequeue(output->q_out, (void **) &packet) != 0)
                        continue;
                output->count--;

                /* this packet made it to the endpoint */
                admitted_insert_admitted_edge(admitted, packet->src,
                                              packet->dst, packet->id);

                /* return the packet to the mempool */
                fp_mempool_put(state->packet_mempool, packet);
        }

        /* move packets from main input queue to individual output queues
           these are packets that arrived at the router during this timeslot */
        while (fp_ring_dequeue(router->q_in, (void **) &packet) == 0) {
                /* assume this is a router with only downward-facing links to
                   endpoints */
                output = &router->endpoint_outputs[packet->dst];

                if (output->count == output->capacity) {
                        /* this packet should be dropped */
                        admitted_insert_dropped_edge(admitted, packet->src,
                                                     packet->dst, packet->id);

                        /* return the packet to the mempool */
                        fp_mempool_put(state->packet_mempool, packet);

                        continue;
                }

                while (fp_ring_enqueue(output->q_out, packet) == -ENOBUFS)
                        printf("error: failed enqueue within router\n");
                output->count++;
        }


        /* send out the admitted traffic */
        while (fp_ring_enqueue(state->q_admitted_out, admitted) != 0)
                printf("error: cannot enqueue admitted traffic\n");
}
