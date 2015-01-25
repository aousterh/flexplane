/*
 * probdrop.cc
 *
 * Created on: January 25, 2015
 *     Author: hari
 */

#include "queue_managers/probdrop.h"
#include "api_impl.h"
#include "../graph-algo/random.h"

#define PROBDROP_QUEUE_CAPACITY 4096

ProbDropQueueManager::ProbDropQueueManager(PacketQueueBank *bank,
                                 struct probdrop_args *probdrop_params, Dropper &dropper)
    : m_bank(bank), m_dropper(dropper), m_probdrop_params(*probdrop_params)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
    seed_random(&random_state, time(NULL));
}

void ProbDropQueueManager::enqueue(struct emu_packet *pkt,
                                     uint32_t port, uint32_t queue)
{
    uint32_t qlen = m_bank->occupancy(port, queue);
    if (qlen >= m_probdrop_params.q_capacity) {
        /* no space to enqueue, drop this packet */
        adm_log_emu_router_dropped_packet(&g_state->stat);
        m_dropper.drop(pkt, port);
	return;
    }

    if (random_int(&random_state, RANDRANGE_16) <=  (uint16_t)(m_probdrop_params.p_drop*RANDRANGE_16)) {
        // drop this packet
        adm_log_emu_router_dropped_packet(&g_state->stat);
	m_dropper.drop(pkt, port);
    } else {
        m_bank->enqueue(port, queue, pkt);
    }
}
    
/**
 * All ports of a ProbDropRouter run ProbDrop. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
ProbDropRouter::ProbDropRouter(uint16_t id, struct probdrop_args *probdrop_params, Dropper &dropper,
		struct queue_bank_stats *stats)
    : m_bank(EMU_ROUTER_NUM_PORTS, 1, PROBDROP_QUEUE_CAPACITY, stats),
      m_rt(16, 0, EMU_ROUTER_NUM_PORTS, 0),
	  m_cla(),
      m_qm(&m_bank, probdrop_params, dropper),
      m_sch(&m_bank),
      ProbDropRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ROUTER_NUM_PORTS)
{}

ProbDropRouter::~ProbDropRouter() {}
