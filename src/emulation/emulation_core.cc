/*
 * emulation_core.cc
 *
 *  Created on: February 12, 2014
 *      Author: aousterh
 */

#include "emulation_core.h"
#include "drivers/EndpointDriver.h"
#include "drivers/RouterDriver.h"

EmulationCore::EmulationCore(EndpointDriver **epg_drivers,
		RouterDriver **router_drivers, uint16_t n_epgs, uint16_t n_rtrs,
		uint16_t core_index, struct fp_ring *q_admitted_out,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_mempool *packet_mempool)
	: m_out(q_admitted_out, admitted_traffic_mempool, packet_mempool, &m_stat),
	  m_n_epgs(n_epgs),
	  m_n_rtrs(n_rtrs),
	  m_core_index(core_index),
	  m_dropper(m_out, &m_stat)
{
	uint32_t i;

	m_endpoint_drivers.reserve(m_n_epgs);
	for (i = 0; i < n_epgs; i++) {
		m_endpoint_drivers.push_back(epg_drivers[i]);
		m_endpoint_drivers[i]->assign_to_core(&m_out, &m_dropper, &m_stat,
				core_index);
	}

	m_router_drivers.reserve(m_n_rtrs);
	for (i = 0; i < n_rtrs; i++) {
		m_router_drivers.push_back(router_drivers[i]);
		m_router_drivers[i]->assign_to_core(&m_dropper, &m_stat, core_index);
	}

	/* initialize log to zeroes */
	memset(&m_stat, 0, sizeof(struct emu_admission_core_statistics));
}

void EmulationCore::step() {
	uint32_t i;

	/* push/pull at endpoints and routers must be done in a specific order to
	 * ensure that packets pushed in one timeslot cannot be pulled until the
	 * next. */

	/* push new packets from the network to endpoints */
	for (i = 0; i < m_n_epgs; i++)
		m_endpoint_drivers[i]->step();

	/* emulate one timeslot at each router (push and pull) */
	for (i = 0; i < m_n_rtrs; i++)
		m_router_drivers[i]->step();

	m_out.flush();
}

void EmulationCore::cleanup() {
	uint32_t i;

	/* free all endpoints */
	for (i = 0; i < m_n_epgs; i++) {
		m_endpoint_drivers[i]->cleanup();
		delete m_endpoint_drivers[i];
	}

	/* free all routers */
	for (i = 0; i < m_n_rtrs; i++) {
		m_router_drivers[i]->cleanup();
		delete m_router_drivers[i];
	}
}
