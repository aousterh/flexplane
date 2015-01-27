/*
 * EndpointDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_ENDPOINTDRIVER_H_
#define DRIVERS_ENDPOINTDRIVER_H_

#include "../graph-algo/fp_ring.h"

class EndpointGroup;
class EmulationOutput;
struct emu_admission_statistics;

class EndpointDriver {
public:
	EndpointDriver(struct fp_ring *q_new_packets, struct fp_ring *q_to_router,
			struct fp_ring *q_from_router, struct fp_ring *q_resets,
			EndpointGroup *epg, struct emu_admission_statistics *stat);

	/**
	 * Prepares this driver to run on a specific core.
	 */
	void assign_to_core(EmulationOutput *out);

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

	struct fp_ring *m_q_new_packets;
	struct fp_ring *m_q_to_router;
	struct fp_ring *m_q_from_router; /* must free incoming ring from network */
	struct fp_ring *m_q_resets;
	EndpointGroup *m_epg;
	struct emu_admission_statistics	*m_stat;
};

#endif /* DRIVERS_ENDPOINTDRIVER_H_ */
