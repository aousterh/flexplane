/*
 * SingleRackNetworkDriver.cc
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#include "SingleRackNetworkDriver.h"

#include "../util/make_ring.h"

SingleRackNetworkDriver::SingleRackNetworkDriver(struct fp_ring* q_new_packets,
		EndpointGroup* epg, Router* router, struct fp_ring *q_resets,
		struct emu_admission_statistics* stat, uint32_t ring_size)
    : m_q_epg_to_router(make_ring("epg_to_router_ring", ring_size, 0,
                                  RING_F_SP_ENQ | RING_F_SC_DEQ)),
      m_q_router_to_epg(make_ring("router_to_epg_ring", ring_size, 0,
                                  RING_F_SP_ENQ | RING_F_SC_DEQ)),
      m_endpoint_driver(q_new_packets, m_q_epg_to_router, m_q_router_to_epg,
    		  q_resets, epg, stat),
      m_router_driver(router, m_q_epg_to_router, m_q_router_to_epg, stat)
{}

void SingleRackNetworkDriver::step() {
	m_endpoint_driver.step();
	m_router_driver.step();
}
