/*
 * lstf_qm.h
 *
 *  Created on: November 17, 2015
 *      Author: lut
 */

#ifndef LSTF_QM_H_
#define LSTF_QM_H_

#include "config.h"
#include "emu_topology.h"
#include "router.h"
#include "composite.h"
#include "lstf_queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/LSTFScheduler.h"

#define LSTF_QUEUE_CAPACITY     1024	

struct lstf_args{
        uint16_t q_capacity;  
};

class LSTFQueueManager : public QueueManager {
public:
        LSTFQueueManager(LSTFQueueBank *bank);
        void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
                uint64_t cur_time, Dropper *dropper);

private:
        LSTFQueueBank *m_bank;
};

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier,
                LSTFQueueManager, LSTFScheduler> LSTFRouterBase;

class LSTFRouter : public LSTFRouterBase {
public:
        LSTFRouter(struct lstf_args *lstf_params, uint32_t rack_index,
                     struct emu_topo_config *topo_config);
        virtual struct queue_bank_stats *get_queue_bank_stats();
        virtual ~LSTFRouter();

private:
        LSTFQueueBank m_bank;
        TorRoutingTable m_rt;
        SingleQueueClassifier m_cla;
        LSTFQueueManager m_qm;
        LSTFScheduler m_sch;
};

#endif /* LSTF_QM_H_ */
