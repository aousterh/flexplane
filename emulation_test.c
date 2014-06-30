/*
 * emulation_test.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "packet.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

#define EMULATION_DURATION 7

int main() {
        uint16_t i, num_packet_qs;

        /* initialize state */
        /* packet queues for:
           finished packets, endpoints, router inputs, router outputs */
        num_packet_qs = 1 + EMU_NUM_ENDPOINTS + EMU_NUM_ROUTERS +
                EMU_NUM_ROUTERS * EMU_ROUTER_MAX_ENDPOINT_PORTS;
        struct fp_mempool *packet_mempool;
        struct fp_ring *packet_queues[num_packet_qs];

        packet_mempool = fp_mempool_create(PACKET_MEMPOOL_SIZE,
                                           sizeof(struct emu_packet));
        for (i = 0; i < num_packet_qs; i++) {
                packet_queues[i] = fp_ring_create(PACKET_Q_SIZE);
        }

        struct emu_state *state = emu_create_state(packet_mempool,
                                                   packet_queues);

        /* run a simple test of emulation framework */

        /* add some backlog */
        emu_add_backlog(state, 0, 1, 1, 13);
        emu_add_backlog(state, 0, 3, 3, 27);
        emu_add_backlog(state, 7, 3, 2, 100);

        /* run some timeslots of emulation */
        for (i = 0; i < EMULATION_DURATION; i++)
                emu_timeslot(state);
}
