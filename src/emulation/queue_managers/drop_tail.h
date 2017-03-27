/*
 * drop_tail.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_H_
#define DROP_TAIL_H_

#include "config.h"
#include "emu_topology.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "routing_tables/CoreRoutingTable.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/FlowIDClassifier.h"
#include "classifiers/BySourceClassifier.h"
#include "classifiers/SourceIDClassifier.h"
#include "schedulers/SingleQueueScheduler.h"
#include "schedulers/PriorityScheduler.h"
#include "schedulers/RRScheduler.h"
#include "output.h"

#include <stdexcept>

#define DROP_TAIL_QUEUE_CAPACITY 1024

struct drop_tail_args {
    uint16_t q_capacity;
};

class DropTailQueueManager : public QueueManager {
public:
	DropTailQueueManager(PacketQueueBank *bank, uint32_t queue_capacity);
	inline void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
			uint64_t cur_time, Dropper *dropper);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;
};

inline void DropTailQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t port, uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
	if (m_bank->occupancy(port, queue) >= m_q_capacity) {
		/* no space to enqueue, drop this packet */
		dropper->drop(pkt, port);
	} else {
		m_bank->enqueue(port, queue, pkt);
	}
}

typedef CompositeRouter<TorRoutingTable, FlowIDClassifier, DropTailQueueManager, SingleQueueScheduler>
	DropTailRouterBase;

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public DropTailRouterBase {
public:
    DropTailRouter(uint16_t q_capacity, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~DropTailRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    FlowIDClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

typedef CompositeRouter<CoreRoutingTable, FlowIDClassifier, DropTailQueueManager, SingleQueueScheduler>
	DropTailCoreRouterBase;

/**
 * A drop tail core router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailCoreRouter : public DropTailCoreRouterBase {
public:
    DropTailCoreRouter(uint16_t q_capacity,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~DropTailCoreRouter();

private:
    PacketQueueBank m_bank;
    CoreRoutingTable m_rt;
    FlowIDClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

typedef CompositeRouter<TorRoutingTable, FlowIDClassifier, DropTailQueueManager, PriorityScheduler>
	PriorityByFlowRouterBase;

class PriorityByFlowRouter : public PriorityByFlowRouterBase {
public:
    PriorityByFlowRouter(uint16_t q_capacity, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~PriorityByFlowRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    FlowIDClassifier m_cla;
    DropTailQueueManager m_qm;
    PriorityScheduler m_sch;
};

typedef CompositeRouter<TorRoutingTable, BySourceClassifier, DropTailQueueManager, PriorityScheduler>
	PriorityBySourceRouterBase;

class PriorityBySourceRouter : public PriorityBySourceRouterBase {
public:
    PriorityBySourceRouter(struct prio_by_src_args *args, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~PriorityBySourceRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    BySourceClassifier m_cla;
    DropTailQueueManager m_qm;
    PriorityScheduler m_sch;
};

typedef CompositeRouter<TorRoutingTable, SourceIDClassifier, DropTailQueueManager, RRScheduler>
	RRRouterBase;

class RRRouter : public RRRouterBase {
public:
	RRRouter(uint16_t q_capacity, uint32_t rack_index,
			struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~RRRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SourceIDClassifier m_cla;
    DropTailQueueManager m_qm;
    RRScheduler m_sch;
};

#endif /* DROP_TAIL_H_ */
