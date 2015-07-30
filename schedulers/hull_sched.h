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
#include "queue_managers/hull.h"

class HULLScheduler : public SingleQueueScheduler {
public:
    HULLScheduler(PacketQueueBank *bank, uint32_t n_ports,
    		struct hull_args *hull_params);

	struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time);

	inline void assign_to_core(Dropper *dropper) { m_dropper = dropper; };

private:
    /** the means to mark packets */
    Dropper *m_dropper;

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
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
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
