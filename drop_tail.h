/*
 * drop_tail.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_H_
#define DROP_TAIL_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "classifiers/TorClassifier.h"
#include "schedulers/SingleQueueScheduler.h"
#include "output.h"

#include <stdexcept>

struct packet_queue;

struct drop_tail_args {
    uint16_t q_capacity;
};

class DropTailQueueManager : public QueueManager {
public:
	DropTailQueueManager(PacketQueueBank *bank, uint32_t queue_capacity,
			Dropper &dropper);
	void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;

	/** the means to drop packets */
	Dropper m_dropper;
};

typedef CompositeRouter<TorClassifier, DropTailQueueManager, SingleQueueScheduler>
	DropTailRouterBase;

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public DropTailRouterBase {
public:
    DropTailRouter(uint16_t id, uint16_t q_capacity, Dropper &dropper);
    virtual ~DropTailRouter();

private:
    PacketQueueBank m_bank;
    TorClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* DROP_TAIL_H_ */
