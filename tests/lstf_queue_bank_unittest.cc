/*
 * lstf_queue_bank_unittest.cc
 *
 *  Created on: November 17, 2015
 *      Author: lut
 */

#include "../lstf_queue_bank.h"
#include "../queue_managers/lstf_qm.h"
#include "gtest/gtest.h"

/*
 * Construct a LSTF Queue Bank, check that the port mask is empty,
 * delete it.
 */
TEST(LSTFQueueBankTest, construct_empty) {
        LSTFQueueBank *qb = new LSTFQueueBank(32, LSTF_QUEUE_CAPACITY);
        EXPECT_EQ(0, *qb->non_empty_port_mask());
        delete qb;
}

TEST(LSTFQueueBankTest, enqueue_dequeue_one_packet) {
        struct emu_packet *p_enqueue, *p_dequeue;
        uint32_t port_num = 7;

        LSTFQueueBank *qb = new LSTFQueueBank(32, LSTF_QUEUE_CAPACITY);
        p_enqueue = (struct emu_packet *) malloc(sizeof(struct emu_packet));
        p_enqueue->slack = 10;

        qb->enqueue(port_num, p_enqueue, 0);
        p_dequeue = qb->dequeue_least_slack(port_num, 1);

        /* one time unit passed between enqueue and dequeue */
        EXPECT_EQ(p_dequeue->slack, 9);

        free(p_dequeue);
        delete qb;
}

TEST(LSTFQueueBankTest, enqueue_dequeue_least_slack) {
        uint32_t i;
        struct emu_packet *p;
        uint32_t port_num = 27;
        uint64_t slacks[5] = {1001, 1100, 1150, 1270, 1350};
        uint64_t t_ingresses[5] = {500, 10, 10, 100, 10};
        /* effective slacks: 1501, 1110, 1160, 1370, 1360 */
        uint64_t dequeued_slacks[5];

        LSTFQueueBank *qb = new LSTFQueueBank(32, LSTF_QUEUE_CAPACITY);

        for(i=0; i<5; i++){
                p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
                p->slack = slacks[i];
                qb->enqueue(port_num, p, t_ingresses[i]);
        } 

        for(i=0; i<5; i++){
               dequeued_slacks[i] = qb->dequeue_least_slack(port_num, 600)->slack;
        }

        EXPECT_EQ(510, dequeued_slacks[0]);
        EXPECT_EQ(560, dequeued_slacks[1]);
        EXPECT_EQ(760, dequeued_slacks[2]);
        EXPECT_EQ(770, dequeued_slacks[3]);
        EXPECT_EQ(901, dequeued_slacks[4]);

        delete qb;
}

TEST(LSTFQueueBankTest, enqueue_dequeue_most_slack) {
        uint32_t i;
        struct emu_packet *p;
        uint32_t port_num = 27;
        uint64_t slacks[5] = {1001, 1100, 1150, 1270, 1350};
        uint64_t t_ingresses[5] = {500, 10, 10, 100, 10};
        /* effective slacks: 1501, 1110, 1160, 1370, 1360 */
        uint64_t dequeued_slacks[5];

        LSTFQueueBank *qb = new LSTFQueueBank(32, LSTF_QUEUE_CAPACITY);

        for(i=0; i<5; i++){
                p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
                p->slack = slacks[i];
                qb->enqueue(port_num, p, t_ingresses[i]);
        } 

        for(i=0; i<5; i++){
               dequeued_slacks[i] = qb->dequeue_most_slack(port_num)->slack;
        }

        EXPECT_EQ(1001, dequeued_slacks[0]);
        EXPECT_EQ(1270, dequeued_slacks[1]);
        EXPECT_EQ(1350, dequeued_slacks[2]);
        EXPECT_EQ(1150, dequeued_slacks[3]);
        EXPECT_EQ(1100, dequeued_slacks[4]);

        delete qb;
 
}

TEST(LSTFQueueBankTest, enqueue_dequeue_interleaved) {
        uint32_t i;
        struct emu_packet *p;
        uint32_t port_num = 27;
        uint64_t slacks[5] = {1001, 1100, 1150, 1270, 1350};
        uint64_t t_ingresses[5] = {500, 10, 300, 100, 10};
        /* effective slacks: 1501, 1110, 1450, 1370, 1360 */
        uint64_t dequeue_times[5] = {500, 600, 700, 800, 900};
        uint64_t dequeued_slacks[5];
        
        LSTFQueueBank *qb = new LSTFQueueBank(32, LSTF_QUEUE_CAPACITY);

        for(i=0; i<3; i++){
                p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
                p->slack = slacks[i];
                qb->enqueue(port_num, p, t_ingresses[i]);
        }

        for(i=0; i<2; i++){
                dequeued_slacks[i] = qb->dequeue_least_slack(port_num, dequeue_times[i])->slack; 
        }

        for(i=3; i<5; i++){
                p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
                p->slack = slacks[i];
                qb->enqueue(port_num, p, t_ingresses[i]);
        }

        for(i=2; i<5; i++){
                dequeued_slacks[i] = qb->dequeue_least_slack(port_num, dequeue_times[i])->slack;
        }

        EXPECT_EQ(610, dequeued_slacks[0]);
        EXPECT_EQ(850, dequeued_slacks[1]);
        EXPECT_EQ(660, dequeued_slacks[2]);
        EXPECT_EQ(570, dequeued_slacks[3]);
        EXPECT_EQ(601, dequeued_slacks[4]);
}
