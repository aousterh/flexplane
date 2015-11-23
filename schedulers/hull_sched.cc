/*
 * hull_sched.cc
 *
 * Created on: June 12, 2015
 *     Author: aousterh
 */

#include "schedulers/hull_sched.h"
#include <limits>

HULLScheduler::HULLScheduler(PacketQueueBank *bank, uint32_t n_ports,
		struct hull_args *hull_params)
    : SingleQueueScheduler(bank), m_hull_params(*hull_params)
{
	uint32_t i;

    // HULL_QUEUE_CAPACITY should be smaller than MAXINT/HULL_ATOM_SIZE
    assert(HULL_QUEUE_CAPACITY < std::numeric_limits<int>::max()/HULL_MTU_SIZE);
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");

    /* initialize other state */
    m_phantom_len.reserve(n_ports);
    m_last_phantom_update_time.reserve(n_ports);
    for (i = 0; i < n_ports; i++) {
    	m_phantom_len[i] = 0;
    	m_last_phantom_update_time[i] = 0;
    }
}

struct emu_packet *HULLScheduler::schedule(uint32_t output_port,
		uint64_t cur_time, Dropper *dropper)
{
	struct emu_packet *pkt;

	/* call parent to dequeue packet from single queue */
	pkt = SingleQueueScheduler::schedule(output_port, cur_time, dropper);

    /* drain phantom length for this port */
	m_phantom_len[output_port] -=  m_hull_params.GAMMA * HULL_MTU_SIZE *
			(cur_time - m_last_phantom_update_time[output_port]);
	if (m_phantom_len[output_port] < 0) {
		m_phantom_len[output_port] = 0;
	}
	m_last_phantom_update_time[output_port] = cur_time;

    /* add to phantom length */
	m_phantom_len[output_port] += HULL_MTU_SIZE;

    if (m_phantom_len[output_port] > m_hull_params.mark_threshold) {
    	/* Set ECN mark on packet, then enqueue */
        dropper->mark_ecn(pkt, output_port);
    }

    return pkt;
}

/**
 * All ports of a HULLRouter run HULL. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
HULLSchedRouter::HULLSchedRouter(struct hull_args *hull_params,
		uint32_t rack_index, struct emu_topo_config *topo_config)
    : HULLSchedRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config)),
      m_bank(tor_ports(topo_config), 1, HULL_QUEUE_CAPACITY),
      m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
      m_qm(&m_bank, hull_params->q_capacity),
      m_sch(&m_bank, tor_ports(topo_config) * 1, hull_params)
{}

HULLSchedRouter::~HULLSchedRouter() {}

struct queue_bank_stats *HULLSchedRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

