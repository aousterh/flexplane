/*
 * hull.cc
 *
 * Created on: January 29, 2015
 *     Author: hari
 */

#include "queue_managers/dctcp.h"
#include "queue_managers/hull.h"
#include <limits>

HULLQueueManager::HULLQueueManager(PacketQueueBank *bank,
		struct hull_args *hull_params)
    : m_bank(bank), m_hull_params(*hull_params), m_phantom_len(0),
      m_last_phantom_update_time(0)
{
    // HULL_QUEUE_CAPACITY should be smaller than MAXINT/HULL_ATOM_SIZE
    assert(HULL_QUEUE_CAPACITY < std::numeric_limits<int>::max()/HULL_MTU_SIZE);
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
}

void HULLQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
		uint32_t queue, uint64_t time)
{
    uint32_t qlen = m_bank->occupancy(port, queue);
    if (qlen >= m_hull_params.q_capacity) {
        /* no space to enqueue, drop this packet */
        m_dropper->drop(pkt, port);
        return;
    }

    /* drain phantom length if we haven't already in this timeslot */
    if (time > m_last_phantom_update_time) {
    	m_phantom_len -= (time - m_last_phantom_update_time) *
    			m_hull_params.GAMMA * HULL_MTU_SIZE;
    	if (m_phantom_len < 0) {
    		m_phantom_len = 0;
    	}
    	m_last_phantom_update_time = time;
    }

    /* add to phantom length */
	m_phantom_len += HULL_MTU_SIZE;

    if (m_phantom_len > m_hull_params.mark_threshold) {
    	/* Set ECN mark on packet, then enqueue */
        m_dropper->mark_ecn(pkt, port);
    }
    m_bank->enqueue(port, queue, pkt);
}


/**
 * All ports of a HULLRouter run HULL. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
HULLRouter::HULLRouter(uint16_t id, struct hull_args *hull_params)
    : m_bank(EMU_ENDPOINTS_PER_RACK, 1, HULL_QUEUE_CAPACITY),
      m_rt(16, 0, EMU_ENDPOINTS_PER_RACK, 0),
	  m_cla(),
      m_qm(&m_bank, hull_params),
      m_sch(&m_bank),
      HULLRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

HULLRouter::~HULLRouter() {}

void HULLRouter::assign_to_core(Dropper *dropper,
                                struct emu_admission_core_statistics *stat) {
    m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *HULLRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

