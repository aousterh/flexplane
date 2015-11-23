/*
 * drop_tail_tso.cc
 *
 *  Created on: September 28, 2015
 *      Author: aousterh
 */

#include "queue_managers/drop_tail_tso.h"
#include "queue_managers/drop_tail.h"
#include "schedulers/SingleQueueTSOScheduler.h"

DropTailTSOQueueManager::DropTailTSOQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity)
	: m_bank(bank), m_q_capacity(queue_capacity)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

DropTailTSORouter::DropTailTSORouter(uint16_t q_capacity, uint32_t rack_index,
		struct emu_topo_config *topo_config)
	: DropTailTSORouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config)),
	  m_bank(tor_ports(topo_config), 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank, tor_ports(topo_config) * 1)
{}

DropTailTSORouter::~DropTailTSORouter() {}

struct queue_bank_stats *DropTailTSORouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}
