/*
 * router.c
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#include "router.h"

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
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
                output->count--;

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

                /* check if this packet should be dropped */
                if (output->count == output->capacity) {
                        packet_mark_as_dropped(packet);

                        /* enqueue to the finished packet q */
                        while (fp_ring_enqueue(finished_packet_q, packet)
                               == -ENOBUFS)
                                printf("error: failed enqueue to finished packet q\n");

                        continue;
                }

                while (fp_ring_enqueue(output->q_out, packet) == -ENOBUFS)
                        printf("error: failed enqueue within router\n");
                output->count++;
        }
}
