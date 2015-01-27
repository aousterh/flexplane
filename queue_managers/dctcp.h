/*
 * dctcp.h
 *
 *  Created on: December 29, 2014
 *      Author: hari
 */

#ifndef DCTCP_H_
#define DCTCP_H_

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

struct dctcp_args {
    uint16_t q_capacity;
    uint32_t K_threshold;
};


class DCTCPQueueManager : public QueueManager {
public:
    DCTCPQueueManager(PacketQueueBank *bank, struct dctcp_args *dctcp_params);
	inline void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue);

private:
    /** the QueueBank where packets are stored */
    PacketQueueBank *m_bank;
    /** the means to drop packets */
    Dropper *m_dropper;

    struct dctcp_args m_dctcp_params;
	struct emu_admission_core_statistics *m_stat;
};

inline void DCTCPQueueManager::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat)
{
	m_dropper = dropper;
	m_stat = stat;
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, DCTCPQueueManager, SingleQueueScheduler>
	DCTCPRouterBase;

/**
 * A simple DCTCP router.
 * @output_queue: a queue of packets for each output port
 */
class DCTCPRouter : public DCTCPRouterBase {
public:
    DCTCPRouter(uint16_t id, struct dctcp_args *dctcp_params,
    		struct queue_bank_stats *stats);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    virtual ~DCTCPRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    DCTCPQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* DCTCP_H_ */
