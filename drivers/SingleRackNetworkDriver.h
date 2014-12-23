/*
 * SingleRackNetworkDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_SINGLERACKNETWORKDRIVER_H_
#define DRIVERS_SINGLERACKNETWORKDRIVER_H_

struct fp_ring;
#include "EndpointDriver.h"
#include "RouterDriver.h"
#include <stdint.h>

/**
 * An emulation driver for a simple network, with only a single rack:
 *   one EndpointGroup with all rack endpoints, and the top-of-rack router
 */
class SingleRackNetworkDriver {
public:
	/**
	 * c'tor
	 * @param q_new_packets: a ring where new packets from endpoints are input
	 * @param epg: the endpoint group
	 * @param router: the top-of-rack router
	 * @param stat: where to collect statistics
	 * @param ring_size: the size of rings between endpoint group and router,
	 * 	 	must be a power of 2.
	 */
	SingleRackNetworkDriver(struct fp_ring *q_new_packets,
			EndpointGroup *epg, Router *router,
			struct emu_admission_statistics *stat, uint32_t ring_size);

	/**
	 * Perform one step of the emulation
	 */
	void step();

private:
	struct fp_ring *m_q_epg_to_router;
	struct fp_ring *m_q_router_to_epg;
	EndpointDriver m_endpoint_driver;
	RouterDriver m_router_driver;
};

#endif /* DRIVERS_SINGLERACKNETWORKDRIVER_H_ */
