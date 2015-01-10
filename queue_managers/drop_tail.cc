/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "queue_managers/drop_tail.h"

#define DROP_TAIL_QUEUE_CAPACITY 4096

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity, Dropper &dropper, enum component_type type)
	: m_bank(bank), m_q_capacity(queue_capacity), m_dropper(dropper),
	  m_type(type)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

DropTailRouter::DropTailRouter(uint16_t q_capacity, Dropper &dropper,
		struct queue_bank_stats *stats)
	: m_bank(EMU_ROUTER_NUM_PORTS, 1, DROP_TAIL_QUEUE_CAPACITY, stats),
	  m_rt(16, 0, EMU_ROUTER_NUM_PORTS, 0),
	  m_cla(),
	  m_qm(&m_bank, q_capacity, dropper, TYPE_ROUTER),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ROUTER_NUM_PORTS)
{}

DropTailRouter::~DropTailRouter() {}
