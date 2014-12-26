/*
 * simple_endpoint.cc
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#include "simple_endpoint.h"
#include "api.h"
#include "api_impl.h"

#define SIMPLE_ENDPOINT_QUEUE_CAPACITY 4096

SimpleEndpointGroup::SimpleEndpointGroup(uint16_t num_endpoints,
		EmulationOutput& emu_output, uint16_t start_id, uint16_t q_capacity)
: m_bank(num_endpoints, 1, SIMPLE_ENDPOINT_QUEUE_CAPACITY),
  m_emu_output(emu_output),
  m_dropper(m_emu_output),
  m_cla(),
  m_qm(&m_bank, q_capacity, m_dropper),
  m_sch(&m_bank),
  m_sink(m_emu_output),
  SimpleEndpointGroupBase(&m_cla, &m_qm, &m_sch, &m_sink, start_id, num_endpoints)
{}

void SimpleEndpointGroup::reset(uint16_t endpoint_id)
{
	/* dequeue all queued packets */
	while (!m_bank.empty(endpoint_id, 0))
		m_emu_output.free_packet(m_bank.dequeue(endpoint_id, 0));
}
