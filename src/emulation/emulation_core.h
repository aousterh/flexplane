/*
 * emulation_core.h
 *
 *  Created on: February 12, 2015
 *      Author: aousterh
 */

#ifndef EMULATION_CORE_H_
#define EMULATION_CORE_H_

#include "admissible_log.h"
#include "config.h"
#include "output.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include <inttypes.h>
#include <vector>

class EmulationOutput;
class EndpointDriver;
class RouterDriver;

/**
 * A class to encapsulate the state used by one core in the emulation.
 * @m_out: a class for communicating admitted/dropped packets to comm cores
 * @m_endpoint_drivers: one driver for each endpoint group in the network
 * @m_n_epgs: number of endpoint groups this core controls
 * @m_router_drivers: one driver for each router in the network
 * @m_n_rtrs: number of routers this core controls
 * @m_stats: stats for this core
 * @m_core_index: index of this core
 */
class EmulationCore {
public:
	EmulationCore(EndpointDriver **epg_drivers, RouterDriver **router_drivers,
			uint16_t num_epgs, uint16_t num_routers, uint16_t core_index,
			struct fp_ring *q_admitted_out,
			struct fp_mempool *admitted_traffic_mempool,
			struct fp_mempool *packet_mempool);

	void step();
	void cleanup();

	inline struct emu_admission_core_statistics *stats() {
		return &m_stat;
	}
	inline struct port_drop_stats *get_port_drop_stats() {
		return m_dropper.get_port_drop_stats();
	}
private:
	EmulationOutput	m_out;
	std::vector<EndpointDriver *> m_endpoint_drivers;
	uint16_t		m_n_epgs;
	std::vector<RouterDriver *> m_router_drivers;
	uint16_t		m_n_rtrs;
	struct emu_admission_core_statistics m_stat;
	uint16_t		m_core_index;
	Dropper			m_dropper;
}  __attribute__((aligned(64))) /* don't want sharing between cores */;

#endif /* EMULATION_CORE_H_ */
