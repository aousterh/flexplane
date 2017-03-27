/*
 * hull_sched.h
 *
 *  Created on: June 12, 2015
 *      Author: aousterh
 */

#ifndef HULL_SCHED_H_
#define HULL_SCHED_H_

#include "config.h"
#include "emu_topology.h"
#include "router.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/dctcp.h"

#define HULL_QUEUE_CAPACITY	DCTCP_QUEUE_CAPACITY
#define HULL_MTU_SIZE      1500

struct hull_args {
    uint16_t q_capacity;
    uint32_t mark_threshold;
    float    GAMMA;
};

class HULLScheduler : public SingleQueueScheduler {
public:
    HULLScheduler(PacketQueueBank *bank, uint32_t n_ports,
    		struct hull_args *hull_params);

	struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time,
			Dropper *dropper);

private:
    struct hull_args m_hull_params;

    /* state for each port */
    std::vector<int32_t>	m_phantom_len;
    std::vector<uint64_t>	m_last_phantom_update_time;
};

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier,
		DropTailQueueManager, HULLScheduler> HULLSchedRouterBase;

/**
 * A simple HULL router.
 * @output_queue: a queue of packets for each output port
 */
class HULLSchedRouter : public HULLSchedRouterBase {
public:
    HULLSchedRouter(struct hull_args *hull_params, uint32_t rack_index,
    		struct emu_topo_config *topo_config);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~HULLSchedRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    DropTailQueueManager m_qm;
    HULLScheduler m_sch;
};

#endif /* HULL_SCHED_H_ */
