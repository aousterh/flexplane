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
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/FlowIDClassifier.h"
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
	inline void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;

	/** the means to drop packets */
	Dropper m_dropper;
};

typedef CompositeRouter<TorRoutingTable, FlowIDClassifier, DropTailQueueManager, SingleQueueScheduler>
	DropTailRouterBase;

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public DropTailRouterBase {
public:
    DropTailRouter(uint16_t q_capacity, Dropper &dropper);
    virtual ~DropTailRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    FlowIDClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

inline void DropTailQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t port, uint32_t queue)
{
	if (m_bank->occupancy(port, queue) >= m_q_capacity) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_router_dropped_packet(&g_state->stat);
		m_dropper.drop(pkt);
	} else {
		m_bank->enqueue(port, queue, pkt);
	}
}

#endif /* DROP_TAIL_H_ */
