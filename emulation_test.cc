/*
 * emulation_test.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "admitted.h"
#include "api_impl.h"
#include "config.h"
#include "emulation.h"
#include "emulation_impl.h"
#include "endpoint.h"
#include "packet.h"
#include "queue_bank_log.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/red.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "drivers/SingleRackNetworkDriver.h"

#include <stdio.h>

class EmulationTest {
public:
    EmulationTest(enum RouterType rtype)
        : packet_mempool(fp_mempool_create("packet_mempool",
        		PACKET_MEMPOOL_SIZE, EMU_ALIGN(sizeof(struct emu_packet)), 0,
        		0, 0)),
          admitted_traffic_mempool(fp_mempool_create("admitted_mempool",
        		  ADMITTED_MEMPOOL_SIZE, sizeof(struct emu_admitted_traffic),
        		  0, 0, 0)),
          q_admitted_out(fp_ring_create("",1 << ADMITTED_Q_LOG_SIZE, 0, 0))
	{
            uint16_t i;
            struct drop_tail_args args;
            void *rtr_args;

            /* initialize algo-specific state */
            switch (rtype) {
            case R_DropTail:
                args.q_capacity = 5; // small capacity for testing
                rtr_args = (void *) &args;
                break;
                
            case R_RED:
                args.q_capacity = 40000; // 4 milliseconds at 1 Gbit/s
                struct red_args red_params;
                red_params.q_capacity = 400;
                red_params.ecn = false;
                red_params.min_th = 20; // 200 microseconds
                red_params.max_th = 200; // 2 milliseconds
                red_params.max_p = 0.05;
                red_params.wq_shift = 8;

                rtr_args = (void *) &red_params;
                break;

            default:
            	printf("Router type %d not supported in emulation test.\n", rtype);
            }


            /* setup emulation state */
            emu_init_state(&state, admitted_traffic_mempool, q_admitted_out,
                           packet_mempool, (1 << PACKET_Q_LOG_SIZE),
						   rtype, rtr_args,
                           E_Simple, NULL); /* use default endpoint args */
	}

    /**
     * Emulate one timeslot and print out the admitted and dropped traffic
     */
    void emulate_and_print_admitted() {
        struct emu_admitted_traffic *admitted;

        /* emulate one timeslot */
        emu_emulate(&state);

        /* print out admitted traffic */
        while (fp_ring_dequeue(state.q_admitted_out, (void **) &admitted) == 0) {
        	admitted_print(admitted);
        	fp_mempool_put(state.admitted_traffic_mempool, admitted);
        }

/*        printf("queue bank logging:");
        print_queue_bank_log(&state.queue_bank_stats);*/
    }

public:
    struct emu_state state;
private:
    struct fp_mempool *packet_mempool;
    struct fp_mempool *admitted_traffic_mempool;
    struct fp_ring *q_admitted_out;
};

int main() {
    EmulationTest *test;
    uint16_t i;

    /* run a basic test of emulation framework */
    test = new EmulationTest(R_DropTail);
    printf("\nTEST 1: basic\n");
    /* src, dst, flow, amount, start_id, pointer to additional data */
    emu_add_backlog(&test->state, 0, 1, 0, 1, 0, NULL);
    emu_add_backlog(&test->state, 0, 3, 0, 3, 0, NULL);
    emu_add_backlog(&test->state, 7, 3, 0, 2, 0, NULL);

    for (i = 0; i < 8; i++)
        test->emulate_and_print_admitted();
    delete test;

    /* test drop-tail behavior at routers */
    printf("\nTEST 2: drop-tail\n");
    test = new EmulationTest(R_DropTail);
    for (i = 0; i < 10; i++) {
        emu_add_backlog(&test->state, i, 13, 0, 3, 0, NULL);
        test->emulate_and_print_admitted();
    }
    for (i = 0; i < 10; i++) {
        test->emulate_and_print_admitted();
    }

    /* test RED behavior at routers */
    printf("\nTEST 3: RED\n");
/*    test = new EmulationTest(R_RED);
    for (i = 0; i < 1000; i++) {
        emu_add_backlog(&test->state, i % EMU_NUM_ENDPOINTS, 13, 0, 3, 0, NULL);
        test->emulate_and_print_admitted();
    }
    for (i = 0; i < 100; i++) {
        test->emulate_and_print_admitted();
    }*/

    delete test;
}
