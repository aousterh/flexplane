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
#include "drivers/SingleRackNetworkDriver.h"

#include <stdio.h>

class EmulationTest {
public:
	EmulationTest()
		: packet_mempool(fp_mempool_create(PACKET_MEMPOOL_SIZE, EMU_ALIGN(sizeof(struct emu_packet)))),
		  q_new_packets(fp_ring_create("", 1 << PACKET_Q_LOG_SIZE, 0, 0)),
		  admitted_traffic_mempool(fp_mempool_create(ADMITTED_MEMPOOL_SIZE,
								     sizeof(struct emu_admitted_traffic))),
		  q_admitted_out(fp_ring_create("",1 << ADMITTED_Q_LOG_SIZE, 0, 0)),
		  m_emu_output(q_admitted_out, admitted_traffic_mempool, packet_mempool,
				  &state.stat),
		  m_dropper(m_emu_output),
		  m_epg(EMU_NUM_ENDPOINTS, m_emu_output),
		  m_router(0, NULL, m_dropper),
		  m_driver(q_new_packets, &m_epg, &m_router, &state.stat, 1 << PACKET_Q_LOG_SIZE)
	{
		uint16_t i;
		uint32_t packet_size;
		struct drop_tail_args args;

		/* initialize algo-specific state */
		packet_size = EMU_ALIGN(sizeof(struct emu_packet)) + 0;
		args.port_capacity = 5;

		m_epg.init(0, &args);

		/* setup emulation state */
		struct fp_ring *packet_queues[EMU_NUM_PACKET_QS];

		for (i = 0; i < EMU_NUM_PACKET_QS; i++) {
			packet_queues[i] = fp_ring_create("", 1 << PACKET_Q_LOG_SIZE, 0, 0);
		}
		packet_queues[1] = q_new_packets;

		emu_init_state(&state, admitted_traffic_mempool, q_admitted_out,
				packet_mempool, packet_queues, R_DropTail, &args,
				E_DropTail, &args);

	}

	/**
	 * Emulate one timeslot and print out the admitted and dropped traffic
	 */
	void emulate_and_print_admitted() {
		struct emu_admitted_traffic *admitted;

		/* emulate one timeslot */
		m_driver.step();
		m_emu_output.flush();

		/* print out admitted traffic */
		while (fp_ring_dequeue(state.q_admitted_out, (void **) &admitted) != 0)
			printf("error: cannot dequeue admitted traffic\n");

		admitted_print(admitted);
		fp_mempool_put(state.admitted_traffic_mempool, admitted);
	}

private:
	struct fp_mempool *packet_mempool;
	struct fp_ring *q_new_packets;
	struct fp_mempool *admitted_traffic_mempool;
	struct fp_ring *q_admitted_out;
public:
	struct emu_state state;
private:
	EmulationOutput m_emu_output;
	Dropper m_dropper;
	DropTailEndpointGroup m_epg;
	DropTailRouter m_router;
	SingleRackNetworkDriver m_driver;
};

int main() {
	EmulationTest *test;
	uint16_t i;

	/* run a basic test of emulation framework */
	test = new EmulationTest();
	printf("\nTEST 1: basic\n");
	emu_add_backlog(&test->state, 0, 1, 0, 1);
	emu_add_backlog(&test->state, 0, 3, 0, 3);
	emu_add_backlog(&test->state, 7, 3, 0, 2);

	for (i = 0; i < 8; i++)
		test->emulate_and_print_admitted();
	delete test;

	/* test drop-tail behavior at routers */
	printf("\nTEST 2: drop-tail\n");
	test = new EmulationTest();
	for (i = 0; i < 10; i++) {
		emu_add_backlog(&test->state, i, 13, 0, 3);
		test->emulate_and_print_admitted();
	}
	for (i = 0; i < 10; i++) {
		test->emulate_and_print_admitted();
	}

	delete test;
}
