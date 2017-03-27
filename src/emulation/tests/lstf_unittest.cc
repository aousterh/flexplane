/*
 * lstf_unittest.cc
 *
 * Created on: November 25, 2015
 *     Author: lut
 */

#include "emulation.h"
#include "emulation_container.h"
#include "emu_topology.h"
#include "lstf_queue_bank.h"
#include "queue_managers/lstf_qm.h"
#include "gtest/gtest.h"

/*
 * Test that the highest slack packet is dropped when the queue is full.
 */
TEST(LSTFTest, drop_on_full_queue){
        struct emu_topo_config topo_config;
        struct lstf_args rtr_args;
        EmulationContainer *container;
        u8 areq_data_0[8*3] = {0,0,0,0,0,0,0,50,0,0,0,0,0,0,0,40,0,0,0,0,0,0,0,30};
        u8 areq_data_1[8*3] = {0,0,0,0,0,0,0,60,0,0,0,0,0,0,0,20,0,0,0,0,0,0,0,10};
        struct emu_admitted_traffic *admitted;
        uint16_t i, j;
        uint16_t admitted_ids[5] = {0, 20, 11, 12, 22};
        uint16_t dropped_ids[2] = {10, 21};
        uint16_t num_admitted[5] = {0,1,1,1,1};
        uint16_t num_dropped[5] = {1,1,0,0,0};


        topo_config.num_racks = 1;
        topo_config.rack_shift = 5;
        topo_config.num_core_rtrs = 0;

        rtr_args.q_capacity = 2;

        container = new EmulationContainer(ADMITTED_MEMPOOL_SIZE,
                        (1 << ADMITTED_Q_LOG_SIZE), PACKET_MEMPOOL_SIZE,
                        (1 << PACKET_Q_LOG_SIZE), R_LSTF, &rtr_args, E_Simple, NULL,
                        &topo_config);

        container->add_backlog(12, 14, 0, 3, 20, &areq_data_0[0]);
        container->add_backlog(13, 14, 0, 3, 10, &areq_data_1[0]);

        for (i = 0; i < 2; i++){
                container->step();
                admitted = container->get_admitted();
                admitted_print(admitted);
                EXPECT_EQ(0, admitted->size);
                container->free_admitted(admitted);
        }

        for (i = 0; i < 5; i++){
                container->step();

                admitted = container->get_admitted();

                EXPECT_EQ(num_admitted[i], admitted->admitted);
                EXPECT_EQ(num_dropped[i], admitted->dropped);

                for(j=0; j<admitted->size; j++) {
                        if (admitted->edges[j].flags == EMU_FLAGS_DROP)
                                EXPECT_EQ(dropped_ids[i], admitted->edges[j].id);
                        else
                                EXPECT_EQ(admitted_ids[i], admitted->edges[j].id);
                }

                container->free_admitted(admitted);
        }

        delete container;
}
