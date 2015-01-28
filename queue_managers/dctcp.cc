/*
 * dctcp.cc
 *
 * Created on: December 29, 2014
 *     Author: hari
 */

#include "queue_managers/dctcp.h"
#include "api_impl.h"

#define DCTCP_QUEUE_CAPACITY 4096

DCTCPQueueManager::DCTCPQueueManager(PacketQueueBank *bank,
                                 struct dctcp_args *dctcp_params)
    : m_bank(bank), m_dctcp_params(*dctcp_params)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
}

void DCTCPQueueManager::enqueue(struct emu_packet *pkt,
                                     uint32_t port, uint32_t queue)
{
    uint32_t qlen = m_bank->occupancy(port, queue);
    if (qlen >= m_dctcp_params.q_capacity) {
        /* no space to enqueue, drop this packet */
        adm_log_emu_router_dropped_packet(m_stat);
        m_dropper->drop(pkt, port);
	return;
    }

    if (qlen >= m_dctcp_params.K_threshold) {
      /* Set ECN mark on packet, then drop into enqueue */
		adm_log_emu_router_marked_packet(m_stat);
        packet_mark_ecn(pkt);
    }

    m_bank->enqueue(port, queue, pkt);
}
    
/**
 * All ports of a DCTCPRouter run DCTCP. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
DCTCPRouter::DCTCPRouter(uint16_t id, struct dctcp_args *dctcp_params,
		struct queue_bank_stats *stats)
    : m_bank(EMU_ENDPOINTS_PER_RACK, 1, DCTCP_QUEUE_CAPACITY, stats),
      m_rt(16, 0, EMU_ENDPOINTS_PER_RACK, 0),
	  m_cla(),
      m_qm(&m_bank, dctcp_params),
      m_sch(&m_bank),
      DCTCPRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

DCTCPRouter::~DCTCPRouter() {}

void DCTCPRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}
