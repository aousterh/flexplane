/*
 * pfabric_unittest.cc
 *
 *  Created on: September 23, 2015
 *      Author: aousterh
 */

#include "emulation.h"
#include "emulation_container.h"
#include "emu_topology.h"
#include "queue_managers/drop_tail.h"
#include "schedulers/RRScheduler.h"
#include "gtest/gtest.h"

/*
 * Test that packets from different flows are dequeued in the right order.
 */
TEST(RoundRobinTest, two_flows) {
	struct emu_topo_config topo_config;
	struct drop_tail_args rtr_args;
	EmulationContainer *container;
	struct emu_admitted_traffic *admitted;
	uint16_t i;
	uint16_t expected_ids_0[9] = {20, 10, 21, 11, 22, 12, 23, 13, 24};
	uint16_t expected_ids_1[9] = {10, 20, 11, 21, 12, 22, 13, 23, 24};
	uint16_t *expected_ids;

	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */

	/* initialize router arguments */
	rtr_args.q_capacity = 32;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_RR, &rtr_args, E_Simple, NULL,
			&topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 5, 20, NULL);
	container->add_backlog(13, 14, 0, 4, 10, NULL);

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
		EXPECT_EQ(1, admitted->size);

		/* the choice of which goes first is random */
		if (i == 0 && admitted->edges[0].src == 12)
			expected_ids = &expected_ids_0[0];
		else if (i == 0)
			expected_ids = &expected_ids_1[0];

		EXPECT_EQ(expected_ids[i], admitted->edges[0].id);

		container->free_admitted(admitted);
	}

	delete container;
}


/*
 * Test that packets are dequeued in the right order with multiple ports active
 * at the same time.
 */
TEST(RoundRobinTest, two_ports) {
	struct emu_topo_config topo_config;
	struct drop_tail_args rtr_args;
	EmulationContainer *container;
	struct emu_admitted_traffic *admitted;
	uint16_t i;
	uint16_t expected_ids_0[9] = {20, 10, 21, 11, 22, 12, 23, 13, 24};
	uint16_t expected_ids_1[9] = {10, 20, 11, 21, 12, 22, 13, 23, 24};
	uint16_t *expected_ids;

	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */

	/* initialize router arguments */
	rtr_args.q_capacity = 32;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_RR, &rtr_args, E_Simple, NULL,
			&topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 5, 20, NULL);
	container->add_backlog(13, 14, 0, 4, 10, NULL);
	container->add_backlog(15, 16, 0, 5, 0, NULL);

	/* first 3 admitted structs should be empty */
	for (i = 0; i < 3; i++) {
		container->step();

		/* check that admitted is empty */
		admitted = container->get_admitted();
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	for (i = 0; i < 5; i++) {
		container->step();

		/* check admitted */
		admitted = container->get_admitted();
		EXPECT_EQ(2, admitted->size);

		/* the choice of which goes first is random */
		if (i == 0 && admitted->edges[0].src == 12)
			expected_ids = &expected_ids_0[0];
		else if (i == 0)
			expected_ids = &expected_ids_1[0];

		/* ports are processed in order, so edge from dst 14 should appear
		 * first */
		EXPECT_EQ(expected_ids[i], admitted->edges[0].id);
		EXPECT_EQ(i, admitted->edges[1].id);

		container->free_admitted(admitted);
	}

	delete container;
}
