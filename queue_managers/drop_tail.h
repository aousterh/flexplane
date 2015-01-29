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
#include "routing_tables/CoreRoutingTable.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/FlowIDClassifier.h"
#include "classifiers/BySourceClassifier.h"
#include "schedulers/SingleQueueScheduler.h"
#include "schedulers/PriorityScheduler.h"
#include "output.h"

#include <stdexcept>

struct packet_queue;

enum component_type {
	TYPE_ROUTER,
	TYPE_ENDPOINT
};

struct drop_tail_args {
    uint16_t q_capacity;
};

class DropTailQueueManager : public QueueManager {
public:
	DropTailQueueManager(PacketQueueBank *bank, uint32_t queue_capacity,
			enum component_type type);
	inline void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
	inline void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
			uint64_t cur_time);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;

	/** the means to drop packets */
	Dropper *m_dropper;

	/** type - router or endpoint, used for logging */
	enum component_type m_type;

	/** stats */
	struct emu_admission_core_statistics *m_stat;
};

inline void DropTailQueueManager::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_dropper = dropper;
	m_stat = stat;
}

inline void DropTailQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t port, uint32_t queue, uint64_t cur_time)
{
	if (m_bank->occupancy(port, queue) >= m_q_capacity) {
		/* no space to enqueue, drop this packet */
		m_dropper->drop(pkt, port);

		/* log the drop */
		if (m_type == TYPE_ROUTER)
			adm_log_emu_router_dropped_packet(m_stat);
		else
			adm_log_emu_endpoint_dropped_packet(m_stat);
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
    DropTailRouter(uint16_t q_capacity, struct queue_bank_stats *stats,
    		uint32_t rack_index);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
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
    DropTailCoreRouter(uint16_t q_capacity, struct queue_bank_stats *stats,
    		uint32_t links_per_tor);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    virtual ~DropTailCoreRouter();

private:
    PacketQueueBank m_bank;
    CoreRoutingTable m_rt;
    FlowIDClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

typedef CompositeRouter<TorRoutingTable, BySourceClassifier, DropTailQueueManager, PriorityScheduler>
	PriorityRouterBase;

class PriorityRouter : public PriorityRouterBase {
public:
    PriorityRouter(uint16_t q_capacity, struct queue_bank_stats *stats,
    		uint32_t rack_index, uint32_t n_hi_prio, uint32_t n_med_prio);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    virtual ~PriorityRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    BySourceClassifier m_cla;
    DropTailQueueManager m_qm;
    PriorityScheduler m_sch;
};

#endif /* DROP_TAIL_H_ */
