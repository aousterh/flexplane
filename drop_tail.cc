/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"

#define DROP_TAIL_QUEUE_CAPACITY 4096

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity, Dropper &dropper)
	: m_bank(bank), m_q_capacity(queue_capacity), m_dropper(dropper)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

DropTailRouter::DropTailRouter(uint16_t q_capacity, Dropper &dropper)
	: m_bank(EMU_ROUTER_NUM_PORTS, 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_rt(16, 0, EMU_ROUTER_NUM_PORTS, 0),
	  m_cla(),
	  m_qm(&m_bank, q_capacity, dropper),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ROUTER_NUM_PORTS)
{}

DropTailRouter::~DropTailRouter() {}
