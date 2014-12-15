/*
 * emulation_test.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "admitted.h"
#include "emulation.h"
#include "packet.h"
#include "drop_tail.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

/**
 * Emulate one timeslot and print out the admitted and dropped traffic
 */
void emulate_and_print_admitted(Emulation *state,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_ring *q_admitted_out) {
	struct emu_admitted_traffic *admitted;

	/* emulate one timeslot */
	state->emulate();

	/* print out admitted traffic */
	while (fp_ring_dequeue(q_admitted_out, (void **) &admitted) != 0)
		printf("error: cannot dequeue admitted traffic\n");

	admitted_print(admitted);
	fp_mempool_put(admitted_traffic_mempool, admitted);
}

/**
 * Setup the state to run an emulation.
 */
Emulation *setup_state(struct fp_mempool **admitted_traffic_mempool,
		struct fp_ring **q_admitted_out) {
	uint16_t i;
	uint32_t packet_size;
	struct drop_tail_args args;

	/* initialize algo-specific state */
	packet_size = EMU_ALIGN(sizeof(struct emu_packet)) + 0;
	args.port_capacity = 5;

	/* setup emulation state */
	struct fp_mempool *packet_mempool;
	struct fp_ring *packet_queues[EMU_NUM_PACKET_QS];
	Emulation *state;

	*admitted_traffic_mempool = fp_mempool_create(ADMITTED_MEMPOOL_SIZE,
						     sizeof(struct emu_admitted_traffic));
	*q_admitted_out = fp_ring_create(ADMITTED_Q_LOG_SIZE);
	packet_mempool = fp_mempool_create(PACKET_MEMPOOL_SIZE, packet_size);
	for (i = 0; i < EMU_NUM_PACKET_QS; i++) {
		packet_queues[i] = fp_ring_create(PACKET_Q_LOG_SIZE);
	}

	state = new Emulation(*admitted_traffic_mempool, *q_admitted_out,
			packet_mempool, packet_queues, &args);

	return state;
}

int main() {
	Emulation *state;
	struct fp_mempool *admitted_traffic_mempool;
	struct fp_ring *q_admitted_out;
	uint16_t i;

	/* run a basic test of emulation framework */
	state = setup_state(&admitted_traffic_mempool, &q_admitted_out);
	printf("\nTEST 1: basic\n");
	state->add_backlog(0, 1, 0, 1);
	state->add_backlog(0, 3, 0, 3);
	state->add_backlog(7, 3, 0, 2);

	for (i = 0; i < 8; i++)
		emulate_and_print_admitted(state, admitted_traffic_mempool,
				q_admitted_out);
	delete state;

	/* test drop-tail behavior at routers */
	printf("\nTEST 2: drop-tail\n");
	state = setup_state(&admitted_traffic_mempool, &q_admitted_out);
	for (i = 0; i < 10; i++) {
		state->add_backlog(i, 13, 0, 3);
		emulate_and_print_admitted(state, admitted_traffic_mempool,
				q_admitted_out);
	}
	for (i = 0; i < 10; i++) {
		emulate_and_print_admitted(state, admitted_traffic_mempool,
				q_admitted_out);
	}

	delete state;
}
