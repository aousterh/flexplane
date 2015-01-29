/*
 * probdrop.h
 *
 *  Created on: January 25, 2015
 *      Author: hari
 */

#ifndef PROBDROP_H_
#define PROBDROP_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/SingleQueueScheduler.h"

struct packet_queue;

struct probdrop_args {
    uint16_t q_capacity;
    float    p_drop;
};


class ProbDropQueueManager : public QueueManager {
public:
    ProbDropQueueManager(PacketQueueBank *bank,
    		struct probdrop_args *probdrop_params);
	inline void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
    		uint64_t cur_time);

private:
    /** the QueueBank where packets are stored */
    PacketQueueBank *m_bank;
    /** the means to drop packets */
    Dropper *m_dropper;

    struct probdrop_args m_probdrop_params;    

    /** Other state **/
    uint32_t random_state; // for random packet drop
    struct emu_admission_core_statistics *m_stat;
};

inline void ProbDropQueueManager::assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat)
{
	m_dropper = dropper;
	m_stat = stat;
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, ProbDropQueueManager, SingleQueueScheduler>
	ProbDropRouterBase;

/**
 * A simple ProbDrop router.
 * @output_queue: a queue of packets for each output port
 */
class ProbDropRouter : public ProbDropRouterBase {
public:
    ProbDropRouter(uint16_t id, struct probdrop_args *probdrop_params,
    		struct queue_bank_stats *stats);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    virtual ~ProbDropRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    ProbDropQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* PROBDROP_H_ */
