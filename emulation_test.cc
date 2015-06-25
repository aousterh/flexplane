/*
 * emulation_test.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation_container.h"
#include "endpoint.h"
#include "router.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/red.h"
#include "queue_managers/dctcp.h"
#include "queue_managers/hull.h"
#include "../graph-algo/generate_requests.h"

EmulationContainer *create_container(enum RouterType rtype) {
	EmulationContainer *container;
	void *rtr_args;
	struct drop_tail_args args;

    /* initialize algo-specific state */
    switch (rtype) {
    case R_DropTail:
    	args.q_capacity = 5; // small capacity for testing
    	rtr_args = (void *) &args;
    	break;

    case R_RED:
    	struct red_args red_params;
    	red_params.q_capacity = 400;
    	red_params.ecn = false;
    	red_params.min_th = 20; // 200 microseconds
    	red_params.max_th = 200; // 2 milliseconds
    	red_params.max_p = 0.05;
    	red_params.wq_shift = 8;

    	rtr_args = (void *) &red_params;
    	break;

    case R_DCTCP:
    	struct dctcp_args dctcp_args;
    	dctcp_args.q_capacity = 512;
    	dctcp_args.K_threshold = 64;
    	rtr_args = (void *) &dctcp_args;

    	printf("router type DCTCP with param: %d\n", dctcp_args.K_threshold);
    	break;

    case R_HULL:
    	struct hull_args hull_args;
    	hull_args.q_capacity = 512;
    	hull_args.mark_threshold = 3000;
    	hull_args.GAMMA = 0.95;
    	rtr_args = (void *) &hull_args;

    	printf("router type HULL with params: %d %d %f\n",
    			hull_args.q_capacity, hull_args.mark_threshold,
    			hull_args.GAMMA);

    	break;

    case R_Prio_by_flow:
    	args.q_capacity = 5; // small capacity for testing
    	rtr_args = (void *) &args;
    	break;

    default:
    	printf("Router type %d not supported in emulation test.\n", rtype);
    }

    /* Initialize container */
    container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
    		(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
    		(1 << PACKET_Q_LOG_SIZE), rtype, rtr_args, E_Simple, NULL);

    return container;
}

int main() {
    EmulationContainer *container;
    uint16_t i;

    /* run a basic test of emulation framework */
    container = create_container(R_DropTail);
    printf("\nTEST 1: basic\n");
    /* src, dst, flow, amount, start_id, pointer to additional data */
    container->add_backlog(0, 1, 0, 1, 0, NULL);
    container->add_backlog(0, 3, 0, 3, 0, NULL);
    container->add_backlog(7, 3, 0, 2, 0, NULL);

    for (i = 0; i < 8; i++) {
        container->step();
        container->print_admitted();
    }
    delete container;

    /* test drop-tail behavior at routers */
    printf("\nTEST 2: drop-tail\n");
    container = create_container(R_DropTail);
    for (i = 0; i < 10; i++) {
    	container->add_backlog(i, 13, 0, 3, 0, NULL);
        container->step();
        container->print_admitted();
    }
    for (i = 0; i < 10; i++) {
        container->step();
        container->print_admitted();
    }
    delete container;

    /* test RED behavior at routers */
    printf("\nTEST 3: RED\n");
    container = create_container(R_RED);
    /* generate some random requests */
    uint32_t max_requests = 1000 * EMU_NUM_ENDPOINTS;
    struct request_info *requests = (struct request_info *)
    		malloc(max_requests * sizeof(struct request_info));
    uint32_t num_requests = generate_requests_poisson(requests, max_requests,
    		EMU_NUM_ENDPOINTS, 1000, 0.5, 10);
    struct request_info *current_request = requests;
    for (i = 0; i < 1000; i++) {
    	while (current_request->timeslot <= i) {
    		container->add_backlog(current_request->src, current_request->dst,
    				0, current_request->backlog, 0, NULL);
    		current_request++;
    	}
        container->step();
        container->print_admitted();
    }
    for (i = 0; i < 100; i++) {
        container->step();
        container->print_admitted();
    }
    delete container;
}
