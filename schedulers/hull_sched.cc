/*
 * hull_sched.cc
 *
 * Created on: June 12, 2015
 *     Author: aousterh
 */

#include "schedulers/hull_sched.h"
#include <limits>

HULLScheduler::HULLScheduler(PacketQueueBank *bank,
		struct hull_args *hull_params)
    : SingleQueueScheduler(bank), m_hull_params(*hull_params),
      m_phantom_len(0), m_last_phantom_update_time(0)
{
    // HULL_QUEUE_CAPACITY should be smaller than MAXINT/HULL_ATOM_SIZE
    assert(HULL_QUEUE_CAPACITY < std::numeric_limits<int>::max()/HULL_MTU_SIZE);
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
}

struct emu_packet *HULLScheduler::schedule(uint32_t output_port,
		uint64_t cur_time)
{
	struct emu_packet *pkt;

	/* call parent to dequeue packet from single queue */
	pkt = SingleQueueScheduler::schedule(output_port, cur_time);

    /* drain phantom length */
	m_phantom_len -= (cur_time - m_last_phantom_update_time) *
			m_hull_params.GAMMA * HULL_MTU_SIZE;
	if (m_phantom_len < 0) {
		m_phantom_len = 0;
	}
	m_last_phantom_update_time = cur_time;

    /* add to phantom length */
	m_phantom_len += HULL_MTU_SIZE;

    if (m_phantom_len > m_hull_params.mark_threshold) {
    	/* Set ECN mark on packet, then enqueue */
        m_dropper->mark_ecn(pkt, output_port);
    }

    return pkt;
}

/**
 * All ports of a HULLRouter run HULL. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
HULLSchedRouter::HULLSchedRouter(uint16_t id, struct hull_args *hull_params)
    : m_bank(EMU_ENDPOINTS_PER_RACK, 1, HULL_QUEUE_CAPACITY),
      m_rt(16, 0, EMU_ENDPOINTS_PER_RACK, 0),
	  m_cla(),
      m_qm(&m_bank, hull_params->q_capacity),
      m_sch(&m_bank, hull_params),
      HULLSchedRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK)
{}

HULLSchedRouter::~HULLSchedRouter() {}

void HULLSchedRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
    m_qm.assign_to_core(dropper, stat);
    m_sch.assign_to_core(dropper);
}

struct queue_bank_stats *HULLSchedRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

