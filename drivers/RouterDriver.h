/*
 * RouterDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_ROUTERDRIVER_H_
#define DRIVERS_ROUTERDRIVER_H_

#include "../graph-algo/fp_ring.h"

class Router;
class Dropper;
struct emu_admission_statistics;

class RouterDriver {
public:
	RouterDriver(Router *router, struct fp_ring *q_to_router,
			struct fp_ring *q_from_router,
			struct emu_admission_statistics *stat);
	/**
	 * Prepares this driver to run on a specific core.
	 */
	void assign_to_core(Dropper *dropper);

	void step();
	void cleanup();
private:
	Router *m_router;
	struct fp_ring *m_q_to_router; /* must free incoming ring from network */
	struct fp_ring *m_q_from_router;
	struct emu_admission_statistics	*m_stat;
	uint32_t m_random;
};

#endif /* DRIVERS_ROUTERDRIVER_H_ */
