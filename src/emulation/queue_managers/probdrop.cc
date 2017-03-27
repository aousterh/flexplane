/*
 * probdrop.cc
 *
 * Created on: January 25, 2015
 *     Author: hari
 */

#include "queue_managers/probdrop.h"
#include "../graph-algo/random.h"

#define PROBDROP_QUEUE_CAPACITY 4096

ProbDropQueueManager::ProbDropQueueManager(PacketQueueBank *bank,
                                 struct probdrop_args *probdrop_params)
    : m_bank(bank), m_probdrop_params(*probdrop_params)
{
    if (bank == NULL)
        throw std::runtime_error("bank should be non-NULL");
    /* initialize other state */
    seed_random(&random_state, time(NULL));
}

void ProbDropQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
		uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
    uint32_t qlen = m_bank->occupancy(port, queue);
    if (qlen >= m_probdrop_params.q_capacity) {
        /* no space to enqueue, drop this packet */
        dropper->drop(pkt, port);
	return;
    }

    if (random_int(&random_state, RANDRANGE_16) <=  (uint16_t)(m_probdrop_params.p_drop*RANDRANGE_16)) {
        // drop this packet
        dropper->drop(pkt, port);
    } else {
        m_bank->enqueue(port, queue, pkt);
    }
}

/**
 * All ports of a ProbDropRouter run ProbDrop. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
ProbDropRouter::ProbDropRouter(struct probdrop_args *probdrop_params,
		uint32_t rack_index, struct emu_topo_config *topo_config)
    : m_bank(tor_ports(topo_config), 1, PROBDROP_QUEUE_CAPACITY),
      m_rt(topo_config->rack_shift, rack_index,
			  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
	  m_cla(),
      m_qm(&m_bank, probdrop_params),
      m_sch(&m_bank),
      ProbDropRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

struct queue_bank_stats *ProbDropRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

ProbDropRouter::~ProbDropRouter() {}
