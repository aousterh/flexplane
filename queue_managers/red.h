/*
 * red.h
 *
 *  New version created on: December 21, 2014
 *      Author: hari
 */

#ifndef RED_H_
#define RED_H_

#include "config.h"
#include "router.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "routing_tables/TorRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "schedulers/SingleQueueScheduler.h"

#define RED_FORCEDROP true	/* used when queue has overflowed, for the caller to force a drop in mark_or_drop */
#define RED_DROPPKT 0		/* red_rules() tells caller that it dropped the packet in RED */
#define RED_ACCEPTMARKED 1      /* red_rules() tells caller that it did not drop pkt, but ECN-marked it */
#define RED_ACCEPTPKT 2		/* red_rules() tells caller that it accepted the pkt without marking */

struct red_args {
    uint16_t q_capacity;
    bool     ecn; // TODO: this should be conveyed in abstract packets
    uint32_t min_th;
    uint32_t max_th;
    float    max_p;
    uint16_t wq_shift; // specified as a right bit-shift; 3 means 1/8, etc.    
};


class REDQueueManager : public QueueManager {
public:
    REDQueueManager(PacketQueueBank *bank, uint32_t n_queues_total,
    		struct red_args *red_params);
	inline void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat);
    void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
    		uint64_t cur_time);
    uint8_t red_rules(struct emu_packet *pkt, uint32_t qlen, uint32_t port,
    		uint32_t queue, uint64_t cur_time);
    uint8_t mark_or_drop(struct emu_packet *pkt, bool force, uint32_t port,
    		uint32_t queue);
	inline struct port_drop_stats *get_port_drop_stats();

private:
    /** the QueueBank where packets are stored */
    PacketQueueBank *m_bank;
    /** the means to drop packets */
    Dropper *m_dropper;

    /* min_th, max_th, and q_avg are maintained as their values * 2^wq_shift
     * so that we can maintain better precision while operating on integers.
     * otherwise, q_avg would remain zero until the queue was at least
     * 2^wq_shift. */
    struct red_args m_red_params;

    /** RED state, tracked separately for each queue **/
    std::vector<uint32_t> m_q_avg; // EWMA-based queue length * 2^wq_shift
    std::vector<uint32_t> m_count_since_last; // number of pkts since last drop or marked one

    /** Other state **/
    uint32_t random_state; // for random drop/mark generation
    struct emu_admission_core_statistics *m_stat;
};

inline void REDQueueManager::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat)
{
	m_dropper = dropper;
	m_stat = stat;
}

inline struct port_drop_stats *REDQueueManager::get_port_drop_stats() {
	return m_dropper->get_port_drop_stats();
}

typedef CompositeRouter<TorRoutingTable, SingleQueueClassifier, REDQueueManager, SingleQueueScheduler>
	REDRouterBase;

/**
 * A RED router.
 * @output_queue: a queue of packets for each output port
 */
class REDRouter : public REDRouterBase {
public:
    REDRouter(uint16_t id, struct red_args *red_params);
    virtual void assign_to_core(Dropper *dropper,
    			struct emu_admission_core_statistics *stat);
	virtual struct queue_bank_stats *get_queue_bank_stats();
    virtual ~REDRouter();

private:
    PacketQueueBank m_bank;
    TorRoutingTable m_rt;
    SingleQueueClassifier m_cla;
    REDQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

#endif /* RED_H_ */
