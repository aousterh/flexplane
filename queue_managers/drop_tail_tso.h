/*
 * drop_tail_tso.h
 *
 *  Created on: September 25, 2015
 *      Author: aousterh
 */

#ifndef DROP_TAIL_TSO_H_
#define DROP_TAIL_TSO_H_

#include "config.h"
#include "emu_topology.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "drop_tail.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/SingleQueueTSOScheduler.h"
#include "output.h"

#include <stdexcept>

class DropTailTSOQueueManager : public QueueManager {
public:
	DropTailTSOQueueManager(PacketQueueBank *bank, uint32_t queue_capacity);
	inline void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
			uint64_t cur_time, Dropper *dropper);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;
};

inline void DropTailTSOQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t port, uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
	if (m_bank->get_tso_occupancy(port, queue) + pkt->n_mtus >= m_q_capacity)
		dropper->drop(pkt, port);
	else {
		m_bank->enqueue(port, queue, pkt);
		m_bank->increment_tso_occupancy(port, queue, pkt->n_mtus);
	}
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier,
		DropTailTSOQueueManager, SingleQueueTSOScheduler>
	DropTailTSORouterBase;

/**
 * A drop tail router that supports TSO.
 * @output_queue: a queue of packets for each output port
 */
class DropTailTSORouter : public DropTailTSORouterBase {
public:
    DropTailTSORouter(uint16_t q_capacity, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~DropTailTSORouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    DropTailTSOQueueManager m_qm;
    SingleQueueTSOScheduler m_sch;
};

#endif /* DROP_TAIL_TSO_H_ */
