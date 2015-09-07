/*
 * pfabric_qm.cc
 *
 * Created on: September 6, 2015
 *     Author: aousterh
 */

#include "queue_managers/pfabric_qm.h"

PFabricQueueManager::PFabricQueueManager(PFabricQueueBank *bank)
    : m_bank(bank)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
}

void PFabricQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
		uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
	if (m_bank->full(port)) {
		uint32_t lowest_priority = m_bank->lowest_priority(port);
		if (lowest_priority <= pkt->priority) {
			/* drop this packet (it's the lowest priority) */
			dropper->drop(pkt, port);
			return;
		} else {
			/* dequeue the lowest prio packet and drop it */
			struct emu_packet *p_low_prio = m_bank->dequeue_lowest_priority(
					port);
			dropper->drop(p_low_prio, port);
		}
	}

    m_bank->enqueue(port, pkt);
}

/**
 * All ports of a PFabricRouter run PFabric. We don't currently support routers with
 * different ports running different QMs or schedulers.
 */
PFabricRouter::PFabricRouter(struct pfabric_args *pfabric_params,
		uint32_t rack_index, struct emu_topo_config *topo_config)
    : m_bank(tor_ports(topo_config), pfabric_params->q_capacity),
      m_rt(topo_config->rack_shift, rack_index,
    		  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
      m_qm(&m_bank),
      m_sch(&m_bank),
      PFabricRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

PFabricRouter::~PFabricRouter() {}

struct queue_bank_stats *PFabricRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}
