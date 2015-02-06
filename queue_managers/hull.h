/*
 * hull.h
 *
 *  Created on: January 29, 2015
 *      Author: hari
 */

#ifndef HULL_H_
#define HULL_H_

#include "api.h"
#include "config.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/SingleQueueScheduler.h"

#define HULL_QUEUE_CAPACITY DCTCP_QUEUE_CAPACITY
#define HULL_ATOM_SIZE      1000

struct hull_args {
    uint16_t q_capacity;
    uint32_t mark_threshold;
    float    GAMMA;
};


class HULLQueueManager : public QueueManager {
public:
    HULLQueueManager(PacketQueueBank *bank, struct hull_args *hull_params);
    inline void assign_to_core(Dropper *dropper,
                               struct emu_admission_core_statistics *stat);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue, uint64_t time);

private:
    /** the QueueBank where packets are stored */
    PacketQueueBank *m_bank;
    /** the means to drop packets */
    Dropper *m_dropper;

    struct hull_args m_hull_params;
    int32_t          m_phantom_len;
    uint64_t         m_last_phantom_update_time;

    struct emu_admission_core_statistics *m_stat;
};

inline void HULLQueueManager::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat)
{
	m_dropper = dropper;
	m_stat = stat;
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, HULLQueueManager, SingleQueueScheduler>
	HULLRouterBase;

/**
 * A simple HULL router.
 * @output_queue: a queue of packets for each output port
 */
class HULLRouter : public HULLRouterBase {
public:
    HULLRouter(uint16_t id, struct hull_args *hull_params,
    		struct queue_bank_stats *stats);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    virtual ~HULLRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    HULLQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* HULL_H_ */
