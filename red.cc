/*
 * red.cc
 *
 * Created on: December 21, 2014
 *     Author: hari
 */

#include "red.h"
#include "api_impl.h"
#include "../graph-algo/random.h"

#define RED_PORT_CAPACITY 128
#define RED_QUEUE_CAPACITY 4096

REDQueueManager::REDQueueManager(PacketQueueBank *bank,
                                 struct red_args *red_params, Dropper &dropper)
    : m_bank(bank), m_dropper(dropper), m_red_params(*red_params)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
    seed_random(&random_state, time(NULL));
    q_avg = 0;             // follows Floyd's 1993 paper pseudocode
    count_since_last = -1; // follows Floyd's 1993 paper pseudocode
}

void REDQueueManager::enqueue(struct emu_packet *pkt,
                                     uint32_t port, uint32_t queue)
{
    uint32_t qlen = m_bank->occupancy(port, queue);

    printf("REDenq: qlen %d\n", qlen);
    if (qlen >= m_red_params.q_capacity) {
        /* no space to enqueue, drop this packet */
      printf("REDenq: force drop qlen %d capacity%d\n", qlen, m_red_params.q_capacity);
        mark_or_drop(pkt, RED_FORCEDROP);
	return;
    } else {
        if (red_rules(pkt, qlen) != RED_DROPPKT) {
	    m_bank->enqueue(port, queue, pkt);
	}
    }
}

#define RAND_RANGE (1<<16)-1
inline uint8_t REDQueueManager::red_rules(struct emu_packet *pkt, uint32_t qlen)
{
    // note that the EWMA weight is specified as a bit shift factor
    uint32_t q_avg = (qlen >> m_red_params.wq_shift) + (((q_avg << m_red_params.wq_shift) - 1 ) >> m_red_params.wq_shift);
    float p_a, p_b;
    bool  accept=true;

    // XXX TODO: handling idle slots since the time the queue became empty

    if (q_avg > m_red_params.max_th) {
        accept = mark_or_drop(pkt, 0);
    } else if (q_avg > m_red_params.min_th) { // in (q_min, q_max]: probabilistic drop/mark
        p_b = m_red_params.max_p * (float)(q_avg - m_red_params.min_th)/(m_red_params.max_th - m_red_params.min_th);
        p_a = p_b / (1 - count_since_last * p_b);
        if (p_a > 1.0 || p_a < 0.0) {
            p_a = 1;
        }

        // mark_or_drop with probability p_a
        if (p_a*RAND_RANGE  <= random_int(&random_state, RAND_RANGE)) {
            accept = mark_or_drop(pkt, false);
        }
    } 
    count_since_last++;
    // in all other cases, q_avg <= q_min, so just return
    return accept;
}

inline uint8_t REDQueueManager::mark_or_drop(struct emu_packet *pkt, bool force_drop) { 
    count_since_last = -1;
    if (force_drop || !(m_red_params.ecn)) {
        adm_log_emu_router_dropped_packet(&g_state->stat);
        m_dropper.drop(pkt);
	return RED_DROPPKT;
    } else {
        /* mark the ECN bit */
        /* XXX TODO */
        return RED_ACCEPTMARKED;
     }
}

/**
 * All ports of a REDRouter run RED. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
REDRouter::REDRouter(uint16_t id, struct red_args *red_params, Dropper &dropper)
    : m_bank(EMU_ROUTER_NUM_PORTS, 1, RED_QUEUE_CAPACITY),
      m_rt(16, 0, EMU_ROUTER_NUM_PORTS, 0),
	  m_cla(),
      m_qm(&m_bank, red_params, dropper),
      m_sch(&m_bank),
      REDRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ROUTER_NUM_PORTS)
{}

REDRouter::~REDRouter() {}
