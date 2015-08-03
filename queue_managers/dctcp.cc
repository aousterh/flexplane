/*
 * dctcp.cc
 *
 * Created on: December 29, 2014
 *     Author: hari
 */

#include "queue_managers/dctcp.h"

DCTCPQueueManager::DCTCPQueueManager(PacketQueueBank *bank,
                                     struct dctcp_args *dctcp_params)
    : m_bank(bank), m_dctcp_params(*dctcp_params)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
}

void DCTCPQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
		uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
    uint32_t qlen = m_bank->occupancy(port, queue);
    if (qlen >= m_dctcp_params.q_capacity) {
        /* no space to enqueue, drop this packet */
        dropper->drop(pkt, port);
        return;
    }

    /* mark if queue occupancy is greater than K */
    if (qlen > m_dctcp_params.K_threshold) {
      /* Set ECN mark on packet, then drop into enqueue */
        dropper->mark_ecn(pkt, port);
    }

    m_bank->enqueue(port, queue, pkt);
}

/**
 * All ports of a DCTCPRouter run DCTCP. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
DCTCPRouter::DCTCPRouter(struct dctcp_args *dctcp_params, uint32_t rack_index,
		struct emu_topo_config *topo_config)
    : m_bank(tor_ports(topo_config), 1, DCTCP_QUEUE_CAPACITY),
      m_rt(topo_config->rack_shift, rack_index,
    		  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
      m_qm(&m_bank, dctcp_params),
      m_sch(&m_bank),
      DCTCPRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

DCTCPRouter::~DCTCPRouter() {}

struct queue_bank_stats *DCTCPRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}
