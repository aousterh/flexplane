/*
 * EndpointDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_ENDPOINTDRIVER_H_
#define DRIVERS_ENDPOINTDRIVER_H_

struct fp_ring;
class EndpointGroup;
struct emu_admission_statistics;

class EndpointDriver {
public:
	EndpointDriver(struct fp_ring *q_new_packets, struct fp_ring *q_to_router,
			struct fp_ring *q_from_router, EndpointGroup *epg,
			struct emu_admission_statistics *stat);

	/**
	 * Emulate a single timeslot
	 */
	void step();

private:
	void push();
	void pull();
	void process_new();

	struct fp_ring *m_q_new_packets;
	struct fp_ring *m_q_to_router;
	struct fp_ring *m_q_from_router;
	EndpointGroup *m_epg;
	struct emu_admission_statistics	*m_stat;
};

#endif /* DRIVERS_ENDPOINTDRIVER_H_ */
