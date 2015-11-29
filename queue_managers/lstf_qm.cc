/*
 * lstf_qm.cc
 *
 * Created on: November 18, 2015a
 *     Author: lut
 */

#include "queue_managers/lstf_qm.h"

LSTFQueueManager::LSTFQueueManager(LSTFQueueBank *bank)
        : m_bank(bank)
{
        if(bank == NULL)
                throw std::runtime_error("bank should be non-NULL");
}

void LSTFQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
                uint32_t queue, uint64_t cur_time, Dropper *dropper)
{
        if (m_bank->full(port)) {
                uint64_t most_slack = m_bank->most_slack(port);
                if (most_slack <= pkt->slack + cur_time) {
                        dropper->drop(pkt, port);
                        return;
                } else {
                        struct emu_packet *p_high_slack = m_bank->dequeue_most_slack(port);
                        dropper->drop(p_high_slack, port);  
                }
        }

        m_bank->enqueue(port, pkt, cur_time);
}

LSTFRouter::LSTFRouter(struct lstf_args *lstf_params, uint32_t rack_index,
                        struct emu_topo_config *topo_config)
    : m_bank(tor_ports(topo_config), lstf_params->q_capacity),
      m_rt(topo_config->rack_shift, rack_index,
                  endpoints_per_rack(topo_config), tor_uplink_mask(topo_config)),
      m_cla(),
      m_qm(&m_bank),
      m_sch(&m_bank),
      LSTFRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, tor_ports(topo_config))
{}

LSTFRouter::~LSTFRouter() {}

struct queue_bank_stats *LSTFRouter::get_queue_bank_stats(){
        return m_bank.get_queue_bank_stats();
}
