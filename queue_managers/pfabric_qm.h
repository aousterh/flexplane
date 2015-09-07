/*
 * pfabric_qm.h
 *
 *  Created on: September 6, 2015
 *      Author: aousterh
 */

#ifndef PFABRIC_QM_H_
#define PFABRIC_QM_H_

#include "config.h"
#include "emu_topology.h"
#include "router.h"
#include "composite.h"
#include "pfabric_queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/PFabricScheduler.h"

#define PFABRIC_QUEUE_CAPACITY	24

struct pfabric_args {
	uint16_t q_capacity;
};

class PFabricQueueManager : public QueueManager {
public:
	PFabricQueueManager(PFabricQueueBank *bank);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
    		uint64_t cur_time, Dropper *dropper);

private:
    /** the QueueBank where packets are stored */
    PFabricQueueBank *m_bank;
};

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier,
		PFabricQueueManager, PFabricScheduler> PFabricRouterBase;

/**
 * A simple DCTCP router.
 * @output_queue: a queue of packets for each output port
 */
class PFabricRouter : public PFabricRouterBase {
public:
	PFabricRouter(struct pfabric_args *pfabric_params, uint32_t rack_index,
			struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~PFabricRouter();

private:
    PFabricQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    PFabricQueueManager m_qm;
    PFabricScheduler m_sch;
};

#endif /* PFABRIC_QM_H_ */
