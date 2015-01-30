/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "queue_managers/drop_tail.h"

#define DROP_TAIL_QUEUE_CAPACITY 4096

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity, enum component_type type)
	: m_bank(bank), m_q_capacity(queue_capacity), m_type(type)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

DropTailRouter::DropTailRouter(uint16_t q_capacity,
		struct queue_bank_stats *stats, uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK*2, 1, DROP_TAIL_QUEUE_CAPACITY, stats),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(),
	  m_qm(&m_bank, q_capacity, TYPE_ROUTER),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK*2)
{}

DropTailRouter::~DropTailRouter() {}

void DropTailRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}

DropTailCoreRouter::DropTailCoreRouter(uint16_t q_capacity,
		struct queue_bank_stats *stats, uint32_t links_per_tor)
	: m_bank(EMU_ENDPOINTS_PER_RACK*2, 1, DROP_TAIL_QUEUE_CAPACITY, stats),
	  m_rt(EMU_RACK_SHIFT, links_per_tor),
	  m_cla(),
	  m_qm(&m_bank, q_capacity, TYPE_ROUTER),
	  m_sch(&m_bank),
	  DropTailCoreRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK*2)
{}

DropTailCoreRouter::~DropTailCoreRouter() {}

void DropTailCoreRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}

PriorityRouter::PriorityRouter(uint16_t q_capacity,
		struct queue_bank_stats* stats, uint32_t rack_index,
		uint32_t n_hi_prio, uint32_t n_med_prio)
	: m_bank(EMU_ENDPOINTS_PER_RACK, 3, DROP_TAIL_QUEUE_CAPACITY, stats),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(n_hi_prio, n_med_prio),
	  m_qm(&m_bank, q_capacity, TYPE_ROUTER),
	  m_sch(&m_bank),
	  PriorityRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

void PriorityRouter::assign_to_core(Dropper* dropper,
		struct emu_admission_core_statistics* stat) {
	m_qm.assign_to_core(dropper, stat);
}

PriorityRouter::~PriorityRouter() {}

RRRouter::RRRouter(uint16_t q_capacity,
		struct queue_bank_stats* stats, uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK, 64, DROP_TAIL_QUEUE_CAPACITY, stats),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(),
	  m_qm(&m_bank, q_capacity, TYPE_ROUTER),
	  m_sch(&m_bank),
	  RRRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

void RRRouter::assign_to_core(Dropper* dropper,
		struct emu_admission_core_statistics* stat) {
	m_qm.assign_to_core(dropper, stat);
}

RRRouter::~RRRouter() {}

