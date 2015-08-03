/*
 * EndpointDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_ENDPOINTDRIVER_H_
#define DRIVERS_ENDPOINTDRIVER_H_

#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

class EndpointGroup;
class EmulationOutput;
class Dropper;
struct emu_admission_statistics;

class EndpointDriver {
public:
	EndpointDriver(struct fp_ring *q_new_packets, struct fp_ring *q_to_router,
			struct fp_ring *q_from_router, struct fp_ring *q_resets,
			EndpointGroup *epg, struct fp_mempool *packet_mempool,
			uint32_t burst_size);

	/**
	 * Prepares this driver to run on a specific core.
	 */
	void assign_to_core(EmulationOutput *out, Dropper *dropper,
			struct emu_admission_core_statistics *stat, uint16_t core_index);

	/**
	 * Emulate a single timeslot
	 */
	void step();

	/**
	 * Cleanup internal state
	 */
	void cleanup();

private:
	void push();
	void pull();
	void process_new();

	struct fp_ring		*m_q_new_packets;
	struct fp_ring		*m_q_to_router;
	struct fp_ring		*m_q_from_router; /* must free incoming ring from network */
	struct fp_ring		*m_q_resets;
	EndpointGroup		*m_epg;
	Dropper				*m_dropper;
	struct emu_admission_core_statistics	*m_stat;
	uint16_t			m_core_index;
	uint64_t			m_cur_time;
	struct fp_mempool	*m_packet_mempool;
	uint32_t			m_burst_size;
};

#endif /* DRIVERS_ENDPOINTDRIVER_H_ */
