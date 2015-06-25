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

DropTailRouter::DropTailRouter(uint16_t q_capacity, uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK*2, 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  (1 << EMU_RACK_SHIFT) - 1),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK*2)
{}

DropTailRouter::~DropTailRouter() {}

void DropTailRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *DropTailRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

DropTailCoreRouter::DropTailCoreRouter(uint16_t q_capacity)
	: m_bank(EMU_CORE_ROUTER_PORTS, 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt((1 << EMU_RACK_SHIFT) - 1),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  DropTailCoreRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_CORE_ROUTER_PORTS)
{}

DropTailCoreRouter::~DropTailCoreRouter() {}

void DropTailCoreRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *DropTailCoreRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityByFlowRouter::PriorityByFlowRouter(uint16_t q_capacity,
		uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK, 3, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  PriorityByFlowRouterBase(&m_rt, &m_cla, &m_qm, &m_sch,
			  EMU_ENDPOINTS_PER_RACK)
{}

void PriorityByFlowRouter::assign_to_core(Dropper* dropper,
		struct emu_admission_core_statistics* stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *PriorityByFlowRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityByFlowRouter::~PriorityByFlowRouter() {}

PriorityBySourceRouter::PriorityBySourceRouter(struct prio_by_src_args *args,
		uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK, 3, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(args->n_hi_prio, args->n_med_prio),
	  m_qm(&m_bank, args->q_capacity),
	  m_sch(&m_bank),
	  PriorityBySourceRouterBase(&m_rt, &m_cla, &m_qm, &m_sch,
			  EMU_ENDPOINTS_PER_RACK)
{}

void PriorityBySourceRouter::assign_to_core(Dropper* dropper,
		struct emu_admission_core_statistics* stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *PriorityBySourceRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

PriorityBySourceRouter::~PriorityBySourceRouter() {}

RRRouter::RRRouter(uint16_t q_capacity, uint32_t rack_index)
	: m_bank(EMU_ENDPOINTS_PER_RACK, 64, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(EMU_RACK_SHIFT, rack_index, EMU_ENDPOINTS_PER_RACK,
			  EMU_ENDPOINTS_PER_RACK),
	  m_cla(),
	  m_qm(&m_bank, q_capacity),
	  m_sch(&m_bank),
	  RRRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

void RRRouter::assign_to_core(Dropper* dropper,
		struct emu_admission_core_statistics* stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *RRRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

RRRouter::~RRRouter() {}

