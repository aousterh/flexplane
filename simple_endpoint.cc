/*
 * simple_endpoint.cc
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#include "simple_endpoint.h"
#include "queue_managers/drop_tail.h"

SimpleEndpointGroup::SimpleEndpointGroup(uint16_t start_id,
		uint16_t q_capacity, struct emu_topo_config *topo_config)
: m_bank(endpoints_per_epg(topo_config), 1, q_capacity),
  m_cla(),
  m_qm(&m_bank, q_capacity),
  m_sch(&m_bank),
  m_sink(start_id >> topo_config->rack_shift, topo_config->rack_shift),
  SimpleEndpointGroupBase(&m_cla, &m_qm, &m_sch, &m_sink, start_id,
		  endpoints_per_epg(topo_config))
{}

void SimpleEndpointGroup::assign_to_core(EmulationOutput *emu_output)
{
	m_emu_output = emu_output;
	m_sink.assign_to_core(emu_output);
}

void SimpleEndpointGroup::reset(uint16_t endpoint_id)
{
	/* dequeue all queued packets */
	while (!m_bank.empty(endpoint_id, 0)) {
		/* note: cur_time of 0 on dequeue might not result in correct behavior,
		 * but schemes that use it (e.g. RED) are unlikely to be run on
		 * endpoints anyway */
		m_emu_output->free_packet(m_bank.dequeue(endpoint_id, 0, 0));
	}
}


RateLimitingEndpointGroup::RateLimitingEndpointGroup(uint16_t start_id,
		uint16_t q_capacity, uint16_t t_btwn_pkts,
		struct emu_topo_config *topo_config)
: m_bank(endpoints_per_epg(topo_config), 1, q_capacity),
  m_cla(),
  m_qm(&m_bank, q_capacity),
  m_sch(&m_bank, endpoints_per_epg(topo_config), t_btwn_pkts),
  m_sink(start_id >> topo_config->rack_shift, topo_config->rack_shift),
  RateLimitingEndpointGroupBase(&m_cla, &m_qm, &m_sch, &m_sink, start_id,
		  endpoints_per_epg(topo_config))
{}

void RateLimitingEndpointGroup::assign_to_core(EmulationOutput *emu_output)
{
	m_emu_output = emu_output;
	m_sink.assign_to_core(emu_output);
}

void RateLimitingEndpointGroup::reset(uint16_t endpoint_id)
{
	/* dequeue all queued packets */
	while (!m_bank.empty(endpoint_id, 0)) {
		/* note: cur_time of 0 on dequeue might not result in correct behavior,
		 * but schemes that use it (e.g. RED) are unlikely to be run on
		 * endpoints anyway */
		m_emu_output->free_packet(m_bank.dequeue(endpoint_id, 0, 0));
	}
}
