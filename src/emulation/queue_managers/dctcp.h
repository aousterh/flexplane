/*
 * dctcp.h
 *
 *  Created on: December 29, 2014
 *      Author: hari
 */

#ifndef DCTCP_H_
#define DCTCP_H_

#include "config.h"
#include "emu_topology.h"
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
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
    		uint64_t cur_time, Dropper *dropper);

private:
    /** the QueueBank where packets are stored */
    PacketQueueBank *m_bank;

    struct dctcp_args m_dctcp_params;
};

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, DCTCPQueueManager, SingleQueueScheduler>
	DCTCPRouterBase;

/**
 * A simple DCTCP router.
 * @output_queue: a queue of packets for each output port
 */
class DCTCPRouter : public DCTCPRouterBase {
public:
    DCTCPRouter(struct dctcp_args *dctcp_params, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
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
