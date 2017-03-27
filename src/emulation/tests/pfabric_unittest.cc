/*
 * pfabric_unittest.cc
 *
 *  Created on: September 7, 2015
 *      Author: aousterh
 */

#include "emulation.h"
#include "emulation_container.h"
#include "emu_topology.h"
#include "pfabric_queue_bank.h"
#include "queue_managers/pfabric_qm.h"
#include "gtest/gtest.h"

/*
 * Test that packets from two different flows are dequeued in the right order.
 */
TEST(PFabricTest, two_flows) {
	struct emu_topo_config topo_config;
	struct pfabric_args rtr_args;
	EmulationContainer *container;
	u8 areq_data_0[4*4] = {0, 0, 0, 10, 0, 0, 0, 9, 0, 0, 0, 8, 0, 0, 0, 7};
	u8 areq_data_1[4*3] = {0, 0, 0, 11, 0, 0, 0, 6, 0, 0, 0, 5};
	struct emu_admitted_traffic *admitted;
	uint16_t i;
	uint16_t expected_ids[7] = {20, 10, 11, 12, 21, 22, 23};


	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */
	topo_config.num_core_rtrs = 0;

	/* initialize router arguments */
	rtr_args.q_capacity = PFABRIC_QUEUE_CAPACITY;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_PFabric, &rtr_args, E_Simple, NULL,
			&topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 4, 20, &areq_data_0[0]);
	container->add_backlog(13, 14, 0, 3, 10, &areq_data_1[0]);

	/* first 3 admitted structs should be empty */
	for (i = 0; i < 3; i++) {
		container->step();

		/* check that admitted is empty */
		admitted = container->get_admitted();
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	for (i = 0; i < 7; i++) {
		container->step();

		/* check admitted */
		admitted = container->get_admitted();
		EXPECT_EQ(1, admitted->size);
		EXPECT_EQ(expected_ids[i], admitted->edges[0].id);
		container->free_admitted(admitted);
	}

	delete container;
}


/*
 * Test that the lowest priority packet is dropped when the queue is full.
 */
TEST(PFabricTest, drop_on_full_queue) {
	struct emu_topo_config topo_config;
	struct pfabric_args rtr_args;
	EmulationContainer *container;
	u8 areq_data_0[4*3] = {0, 0, 0, 10, 0, 0, 0, 9, 0, 0, 0, 8};
	u8 areq_data_1[4*3] = {0, 0, 0, 11, 0, 0, 0, 6, 0, 0, 0, 5};
	struct emu_admitted_traffic *admitted;
	uint16_t i, j;
	uint16_t admitted_ids[5] = {0, 20, 11, 12, 22};
	uint16_t dropped_ids[2] = {10, 21};
	uint16_t num_admitted[5] = {0, 1, 1, 1, 1};
	uint16_t num_dropped[5] = {1, 1, 0, 0, 0};


	/* initialize emulated topology */
	topo_config.num_racks = 1;
	topo_config.rack_shift = 5; /* 32 machines per rack */
	topo_config.num_core_rtrs = 0;

	/* initialize router arguments */
	rtr_args.q_capacity = 2;

	/* initialize emulation container */
	container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
			(1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
			(1 << PACKET_Q_LOG_SIZE), R_PFabric, &rtr_args, E_Simple, NULL,
			&topo_config);

	/* src, dst, flow, amount, start_id, pointer to additional data */
	container->add_backlog(12, 14, 0, 3, 20, &areq_data_0[0]);
	container->add_backlog(13, 14, 0, 3, 10, &areq_data_1[0]);

	/* first 3 admitted structs should be empty */
	for (i = 0; i < 2; i++) {
		container->step();
		/* check that admitted is empty */
		admitted = container->get_admitted();
		admitted_print(admitted);
		EXPECT_EQ(0, admitted->size);
		container->free_admitted(admitted);
	}

	for (i = 0; i < 5; i++) {
		container->step();

		/* check admitted */
		admitted = container->get_admitted();

		EXPECT_EQ(num_admitted[i], admitted->admitted);
		EXPECT_EQ(num_dropped[i], admitted->dropped);

		for (j = 0; j < admitted->size; j++) {
			if (admitted->edges[j].flags == EMU_FLAGS_DROP)
				EXPECT_EQ(dropped_ids[i], admitted->edges[j].id);
			 else
				EXPECT_EQ(admitted_ids[i], admitted->edges[j].id);
		}

		container->free_admitted(admitted);
	}

	delete container;
}
