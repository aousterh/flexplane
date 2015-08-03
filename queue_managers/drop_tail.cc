/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "queue_managers/drop_tail.h"

#define DROP_TAIL_QUEUE_CAPACITY 4096

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity)
	: m_bank(bank), m_q_capacity(queue_capacity)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

DropTailRouter::DropTailRouter(uint16_t q_capacity, uint32_t rack_index,
		struct emu_topo_config *topo_config)
	: m_bank(tor_ports(topo_config), 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

DropTailRouter::~DropTailRouter() {}

struct queue_bank_stats *DropTailRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

DropTailCoreRouter::DropTailCoreRouter(uint16_t q_capacity,
		struct emu_topo_config *topo_config)
	: m_bank(core_router_ports(topo_config), 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt((1 << topo_config->rack_shift) - 1, num_tors(topo_config)),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  DropTailCoreRouterBase(&m_rt, &m_cla, &m_qm, &m_sch,
			  core_router_ports(topo_config))
{}

DropTailCoreRouter::~DropTailCoreRouter() {}

struct queue_bank_stats *DropTailCoreRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityByFlowRouter::PriorityByFlowRouter(uint16_t q_capacity,
		uint32_t rack_index, struct emu_topo_config *topo_config)
	: m_bank(tor_ports(topo_config), 3, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  PriorityByFlowRouterBase(&m_rt, &m_cla, &m_qm, &m_sch,
			  tor_ports(topo_config))
{}

struct queue_bank_stats *PriorityByFlowRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityByFlowRouter::~PriorityByFlowRouter() {}

PriorityBySourceRouter::PriorityBySourceRouter(struct prio_by_src_args *args,
		uint32_t rack_index, struct emu_topo_config *topo_config)
	: m_bank(tor_ports(topo_config), 3, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(args->n_hi_prio, args->n_med_prio),
	  m_qm(&m_bank, args->q_capacity),
	  m_sch(&m_bank),
	  PriorityBySourceRouterBase(&m_rt, &m_cla, &m_qm, &m_sch,
			  tor_ports(topo_config))
{}

struct queue_bank_stats *PriorityBySourceRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityBySourceRouter::~PriorityBySourceRouter() {}

RRRouter::RRRouter(uint16_t q_capacity, uint32_t rack_index,
		struct emu_topo_config *topo_config)
	: m_bank(tor_ports(topo_config), 64, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  RRRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

struct queue_bank_stats *RRRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

RRRouter::~RRRouter() {}

