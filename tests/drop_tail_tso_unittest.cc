/*
 * drop_tail_tso_unittest.cc
 *
 *  Created on: September 28, 2015
 *      Author: aousterh
 */

#include "emulation.h"
#include "emulation_container.h"
#include "emu_topology.h"
#include "queue_managers/drop_tail_tso.h"
#include "gtest/gtest.h"
#include "../protocol/flags.h"

/*
 * Test that packets from the same flow are dequeued at the correct time when
 * 	there is a continuous backlog of packets to send.
 */
TEST(DropTailTSOTest, one_flow_backlogged) {
	struct emu_topo_config topo_config;
	struct drop_tail_args rtr_args;
	EmulationContainer *container;
	struct emu_admitted_traffic *admitted;
	uint16_t i;
	u8 areq_data[2 * 3] = { 4, 0, 1, 0, 3, 0 };

	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */

	/* initialize router arguments */
	rtr_args.q_capacity = 32;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_DropTailTSO, &rtr_args, E_SimpleTSO,
			NULL, &topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 3, 20, &areq_data[0]);

	/* first 3 admitted structs should be empty */
	for (i = 0; i < 3; i++) {
		container->step();

		/* check that admitted is empty */
		admitted = container->get_admitted();
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	for (i = 0; i < 9; i++) {
		container->step();

		/* check admitted */
		admitted = container->get_admitted();

		/* the choice of which goes first is random */
		if ((i == 0) || (i == 4) || (i == 5)) {
			EXPECT_EQ(1, admitted->size);
		}
		else
			EXPECT_EQ(0, admitted->size);

		container->free_admitted(admitted);
	}

	delete container;
}

/*
 * Test that packets from the same flow are dequeued at the correct time when
 * 	the flow is on and then off and then on again.
 */
TEST(DropTailTSOTest, one_flow_on_off) {
	struct emu_topo_config topo_config;
	struct drop_tail_args rtr_args;
	EmulationContainer *container;
	struct emu_admitted_traffic *admitted;
	uint16_t i;
	u8 areq_data[2 * 3] = { 4, 0, 2, 0, 3, 0 };

	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */

	/* initialize router arguments */
	rtr_args.q_capacity = 32;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_DropTailTSO, &rtr_args, E_SimpleTSO,
			NULL, &topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 2, 20, &areq_data[0]);

	/* first 3 admitted structs should be empty */
	for (i = 0; i < 3; i++) {
		container->step();

		/* check that admitted is empty */
		admitted = container->get_admitted();
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	for (i = 0; i < 6; i++) {
		container->step();

		/* check admitted */
		admitted = container->get_admitted();

		/* the choice of which goes first is random */
		if ((i == 0) || (i == 4)) {
			EXPECT_EQ(1, admitted->size);
		}
		else
			EXPECT_EQ(0, admitted->size);

		container->free_admitted(admitted);
	}

	/* add more backlog again */
	container->add_backlog(12, 14, 0, 1, 22, &areq_data[2]);

	/* next 3 should be empty again */
	for (i = 0; i < 3; i++) {
		container->step();

		/* check that admitted is empty */
		admitted = container->get_admitted();
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	/* should be admitted immediately */
	container->step();
	admitted = container->get_admitted();
	EXPECT_EQ(1, admitted->size);
	container->free_admitted(admitted);

	delete container;
}
