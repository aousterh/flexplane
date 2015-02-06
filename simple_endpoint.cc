/*
 * simple_endpoint.cc
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#include "simple_endpoint.h"
#include "queue_managers/drop_tail.h"

SimpleEndpointGroup::SimpleEndpointGroup(uint16_t num_endpoints,
		uint16_t start_id, uint16_t q_capacity)
: m_bank(num_endpoints, 1, q_capacity, NULL),
  m_cla(),
  m_qm(&m_bank, q_capacity),
  m_sch(&m_bank),
  m_sink(start_id >> EMU_RACK_SHIFT),
  SimpleEndpointGroupBase(&m_cla, &m_qm, &m_sch, &m_sink, start_id, num_endpoints)
{}

void SimpleEndpointGroup::assign_to_core(EmulationOutput *emu_output,
		struct emu_admission_core_statistics *stat)
{
	Dropper *dropper = new Dropper(*emu_output, NULL, stat);

	m_emu_output = emu_output;
	m_sink.assign_to_core(emu_output);
	m_qm.assign_to_core(dropper, stat);
}

void SimpleEndpointGroup::reset(uint16_t endpoint_id)
{
	/* dequeue all queued packets */
	while (!m_bank.empty(endpoint_id, 0))
		m_emu_output->free_packet(m_bank.dequeue(endpoint_id, 0));
}
