/*
 * emulation_test.c
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "admitted.h"
#include "emulation.h"
#include "packet.h"
#include "drop_tail_router.h"
#include "drop_tail_endpoint.h"
#include "drop_tail_packet.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

#define ROUTER_OUTPUT_PORT_CAPACITY 5

/**
 * Emulate one timeslot and print out the admitted and dropped traffic
 */
void emulate_and_print_admitted(struct emu_state *state) {
	struct emu_admitted_traffic *admitted;

	/* emulate one timeslot */
	emu_emulate(state);

	/* print out admitted traffic */
	while (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) != 0)
		printf("error: cannot dequeue admitted traffic\n");

	admitted_print(admitted);
	fp_mempool_put(state->admitted_traffic_mempool, admitted);
}

/**
 * Setup the state to run an emulation.
 */
struct emu_state *setup_state() {
	uint16_t i, num_packet_qs;
	uint32_t packet_size;

	/* initialize state */
	/* packet queues for: endpoints, router inputs, router outputs */
	num_packet_qs = EMU_NUM_ENDPOINTS +
		EMU_NUM_ROUTERS * EMU_ROUTER_NUM_PORTS * 2;
	packet_size = EMU_ALIGN(sizeof(struct emu_packet)) +
		drop_tail_packet_ops.priv_size;
	struct fp_mempool *admitted_traffic_mempool;
	struct fp_ring *q_admitted_out;
	struct fp_mempool *packet_mempool;
	struct fp_ring *packet_queues[num_packet_qs];
	struct emu_state *state;

	admitted_traffic_mempool = fp_mempool_create(ADMITTED_MEMPOOL_SIZE,
						     sizeof(struct emu_admitted_traffic));
	q_admitted_out = fp_ring_create(ADMITTED_Q_LOG_SIZE);
	packet_mempool = fp_mempool_create(PACKET_MEMPOOL_SIZE, packet_size);
	for (i = 0; i < num_packet_qs; i++) {
		packet_queues[i] = fp_ring_create(PACKET_Q_LOG_SIZE);
	}

	state = emu_create_state(admitted_traffic_mempool, q_admitted_out,
				 packet_mempool, packet_queues, &drop_tail_endpoint_ops,
				 &drop_tail_router_ops);

	return state;
}

int main() {
	struct emu_state *state;
	uint16_t i;

	/* run a basic test of emulation framework */
	state = setup_state();
	printf("\nTEST 1: basic\n");
	emu_add_backlog(state, 0, 1, 1);
	emu_add_backlog(state, 0, 3, 3);
	emu_add_backlog(state, 7, 3, 2);

	for (i = 0; i < 7; i++)
		emulate_and_print_admitted(state);
	emu_cleanup(state);

	/* test drop-tail behavior at routers */
	state = setup_state();
	printf("\nTEST 2: drop-tail\n");
	for (i = 0; i < 10; i++) {
		emu_add_backlog(state, i, 13, 3);
		emulate_and_print_admitted(state);
	}
	for (i = 0; i < 10; i++)
		emulate_and_print_admitted(state);

	emu_cleanup(state);
}
