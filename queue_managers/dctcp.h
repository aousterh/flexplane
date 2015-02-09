/*
 * dctcp.h
 *
 *  Created on: December 29, 2014
 *      Author: hari
 */

#ifndef DCTCP_H_
#define DCTCP_H_

#include "config.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/SingleQueueScheduler.h"

#define DCTCP_QUEUE_CAPACITY 4096

struct dctcp_args {
    uint16_t q_capacity;
    uint32_t K_threshold;
};

class DCTCPQueueManager : public QueueManager {
public:
    DCTCPQueueManager(PacketQueueBank *bank, struct dctcp_args *dctcp_params);
    inline void assign_to_core(Dropper *dropper,
                               struct emu_admission_core_statistics *stat);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
    		uint64_t cur_time);
	inline struct port_drop_stats *get_port_drop_stats();

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

inline struct port_drop_stats *DCTCPQueueManager::get_port_drop_stats() {
	return m_dropper->get_port_drop_stats();
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, DCTCPQueueManager, SingleQueueScheduler>
	DCTCPRouterBase;

/**
 * A simple DCTCP router.
 * @output_queue: a queue of packets for each output port
 */
class DCTCPRouter : public DCTCPRouterBase {
public:
    DCTCPRouter(uint16_t id, struct dctcp_args *dctcp_params);
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~DCTCPRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    DCTCPQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* DCTCP_H_ */
