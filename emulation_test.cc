/*
 * emulation_test.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "admitted.h"
#include "api_impl.h"
#include "emulation.h"
#include "emulation_impl.h"
#include "endpoint.h"
#include "packet.h"
#include "drop_tail.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <stdio.h>

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
emu_state *setup_state() {
	uint16_t i;
	uint32_t packet_size;
	struct drop_tail_args args;

	/* initialize algo-specific state */
	packet_size = EMU_ALIGN(sizeof(struct emu_packet)) + 0;
	args.port_capacity = 5;

	/* setup emulation state */
	struct emu_state *state;
	state = (struct emu_state *) malloc(sizeof(struct emu_state));
	struct fp_mempool *packet_mempool;
	struct fp_ring *packet_queues[EMU_NUM_PACKET_QS];
	struct fp_mempool *admitted_traffic_mempool;
	struct fp_ring *q_admitted_out;

	admitted_traffic_mempool = fp_mempool_create(ADMITTED_MEMPOOL_SIZE,
						     sizeof(struct emu_admitted_traffic));
	q_admitted_out = fp_ring_create(ADMITTED_Q_LOG_SIZE);
	packet_mempool = fp_mempool_create(PACKET_MEMPOOL_SIZE, packet_size);
	for (i = 0; i < EMU_NUM_PACKET_QS; i++) {
		packet_queues[i] = fp_ring_create(PACKET_Q_LOG_SIZE);
	}

	emu_init_state(state, admitted_traffic_mempool, q_admitted_out,
			packet_mempool, packet_queues, R_DropTail, &args,
			E_DropTail, &args);

	return state;
}

int main() {
	struct emu_state *state;
	uint16_t i;

	/* run a basic test of emulation framework */
	state = setup_state();
	printf("\nTEST 1: basic\n");
	emu_add_backlog(state, 0, 1, 0, 1);
	emu_add_backlog(state, 0, 3, 0, 3);
	emu_add_backlog(state, 7, 3, 0, 2);

	for (i = 0; i < 8; i++)
		emulate_and_print_admitted(state);
	delete state;

	/* test drop-tail behavior at routers */
	printf("\nTEST 2: drop-tail\n");
	state = setup_state();
	for (i = 0; i < 10; i++) {
		emu_add_backlog(state, i, 13, 0, 3);
		emulate_and_print_admitted(state);
	}
	for (i = 0; i < 10; i++) {
		emulate_and_print_admitted(state);
	}

	delete state;
}
