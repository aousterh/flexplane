/*
 * pfabric_queue_bank_unittest.cc
 *
 *  Created on: September 5, 2015
 *      Author: aousterh
 */

#include "pfabric_queue_bank.h"
#include "queue_managers/pfabric_qm.h"
#include "gtest/gtest.h"

/*
 * Construct a pFabric Queue Bank, check that the port mask is empty,
 * delete it.
 */
TEST(PFabricQueueBankTest, construct_empty) {
	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);
	EXPECT_EQ(0, *qb->non_empty_port_mask());
	delete qb;
}

/*
 * Enqueue one packet and check that dequeue returns the same packet.
 */
TEST(PFabricQueueBankTest, enqueue_dequeue_highest_prio) {
	struct emu_packet *p_enqueue, *p_dequeue;
	uint32_t port_num = 7;

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	p_enqueue = (struct emu_packet *) malloc(sizeof(struct emu_packet));
	p_enqueue->priority = 13;

	/* enqueue then dequeue */
	qb->enqueue(port_num, p_enqueue);
	p_dequeue = qb->dequeue_highest_priority(port_num);

	/* check that we dequeued the original packet */
	EXPECT_EQ(p_enqueue, p_dequeue);
	EXPECT_EQ(13, p_dequeue->priority);

	free(p_dequeue);
	delete qb;
}

/*
 * Enqueue packets with different priorities, check that lowest_priority
 * returns the lowest priority. Also check that dequeue_lowest_priority
 * repeatedly dequeues the correct packet.
 */
TEST(PFabricQueueBankTest, lowest_priority) {
	uint32_t i, lowest_priority;
	struct emu_packet *p;
	uint32_t port_num = 27;
	uint32_t priorities[5] = {19651, 50979, 21943, 2602, 674};
	uint32_t dequeued_priorities[5];

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	/* enqueue several packets */
	for (i = 0; i < 5; i++) {
		p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
		p->priority = priorities[i];
		qb->enqueue(port_num, p);
	}

	/* dequeue and free all packets */
	for (i = 0; i < 5; i++) {
		lowest_priority = qb->lowest_priority(port_num);
		p = qb->dequeue_lowest_priority(port_num);
		dequeued_priorities[i] = p->priority;

		/* check that priority returned by lowest_priority and priority of
		 * lowest priority dequeued packet match */
		EXPECT_EQ(lowest_priority, p->priority);

		free(p);
	}

	/* check that we dequeued packets in the correct order */
	EXPECT_EQ(50979, dequeued_priorities[0]);
	EXPECT_EQ(21943, dequeued_priorities[1]);
	EXPECT_EQ(19651, dequeued_priorities[2]);
	EXPECT_EQ(2602, dequeued_priorities[3]);
	EXPECT_EQ(674, dequeued_priorities[4]);

	delete qb;
}

/*
 * Enqueue packets with different priorities, check that
 * dequeue_highest_priority repeatedly dequeues the correct packet.
 */
TEST(PFabricQueueBankTest, highest_priority_different_flows) {
	uint32_t i;
	struct emu_packet *p;
	uint32_t port_num = 27;
	uint32_t priorities[5] = {674, 19651, 50979, 21943, 2602};
	uint32_t dequeued_priorities[5];

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	/* enqueue several packets */
	for (i = 0; i < 5; i++) {
		p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
		p->priority = priorities[i];
		p->src = i;
		p->dst = i + 1;
		qb->enqueue(port_num, p);
	}

	/* dequeue and free all packets */
	for (i = 0; i < 5; i++) {
		p = qb->dequeue_highest_priority(port_num);
		dequeued_priorities[i] = p->priority;

		free(p);
	}

	/* check that we dequeued packets in the correct order */
	EXPECT_EQ(674, dequeued_priorities[0]);
	EXPECT_EQ(2602, dequeued_priorities[1]);
	EXPECT_EQ(19651, dequeued_priorities[2]);
	EXPECT_EQ(21943, dequeued_priorities[3]);
	EXPECT_EQ(50979, dequeued_priorities[4]);

	delete qb;
}

/*
 * Enqueue packets with different priorities in the same flow, check that
 * dequeue_highest_priority repeatedly dequeues the correct packet.
 */
TEST(PFabricQueueBankTest, highest_priority_one_flow) {
	uint32_t i;
	struct emu_packet *p;
	uint32_t port_num = 27;
	uint32_t priorities[5] = {674, 19651, 50979, 21943, 2602};
	uint32_t ids[5] = {3, 2, 0, 4, 1};
	uint32_t dequeued_ids[5];

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	/* enqueue several packets */
	for (i = 0; i < 5; i++) {
		p = (struct emu_packet *) malloc(sizeof(struct emu_packet));
		p->priority = priorities[i];
		p->src = 3;
		p->dst = 7;
		p->id = ids[i];
		qb->enqueue(port_num, p);
	}

	/* dequeue and free all packets */
	for (i = 0; i < 5; i++) {
		p = qb->dequeue_highest_priority(port_num);
		dequeued_ids[i] = p->id;

		free(p);
	}

	/* check that we dequeued packets in the correct order */
	EXPECT_EQ(0, dequeued_ids[0]);
	EXPECT_EQ(1, dequeued_ids[1]);
	EXPECT_EQ(2, dequeued_ids[2]);
	EXPECT_EQ(3, dequeued_ids[3]);
	EXPECT_EQ(4, dequeued_ids[4]);

	delete qb;
}

/*
 * Enqueue packets with different priorities in a few flows with multiple
 * packets each, check that dequeue_highest_priority repeatedly dequeues the
 * correct packet.
 */
TEST(PFabricQueueBankTest, highest_priority_mixed_flows) {
	uint32_t i;
	struct emu_packet packets[5];
	uint32_t srcs[5] = {0, 3, 3, 0, 3};
	uint32_t dsts[5] = {23, 5, 5, 23, 5};
	uint32_t ids[5] = {5, 27, 28, 6, 29};
	uint32_t prios[5] = {5, 13, 12, 4, 11};
	uint32_t port_num = 27;
	struct emu_packet *dequeued_packets[5];

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	/* enqueue several packets */
	for (i = 0; i < 5; i++) {
		/* initialize packet */
		packets[i].src = srcs[i];
		packets[i].dst = dsts[i];
		packets[i].id = ids[i];
		packets[i].priority = prios[i];

		qb->enqueue(port_num, &packets[i]);
	}

	/* dequeue all packets */
	for (i = 0; i < 5; i++)
		dequeued_packets[i] = qb->dequeue_highest_priority(port_num);

	/* check that we dequeued packets in the correct order */
	EXPECT_EQ(dequeued_packets[0], &packets[0]);
	EXPECT_EQ(dequeued_packets[1], &packets[3]);
	EXPECT_EQ(dequeued_packets[2], &packets[1]);
	EXPECT_EQ(dequeued_packets[3], &packets[2]);
	EXPECT_EQ(dequeued_packets[4], &packets[4]);

	delete qb;
}

/*
 * Test the full function.
 */
TEST(PFabricQueueBankTest, full) {
	uint32_t i;
	uint32_t port_num = 7;
	struct emu_packet p[PFABRIC_QUEUE_CAPACITY];

	PFabricQueueBank *qb = new PFabricQueueBank(32, PFABRIC_QUEUE_CAPACITY);

	/* should be non-full after first n-1 enqueues, then full after nth */
	for (i = 0; i < 23; i++) {
		qb->enqueue(port_num, &p[i]);
		EXPECT_FALSE(qb->full(port_num));
	}

	qb->enqueue(port_num, &p[i]);
	EXPECT_TRUE(qb->full(port_num));

	delete qb;
}
