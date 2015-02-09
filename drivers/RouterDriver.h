/*
 * RouterDriver.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef DRIVERS_ROUTERDRIVER_H_
#define DRIVERS_ROUTERDRIVER_H_

#include "config.h"
#include "../graph-algo/fp_ring.h"

class Router;
class Dropper;
struct emu_admission_statistics;

class RouterDriver {
public:
	RouterDriver(Router *router, struct fp_ring *q_to_router,
			struct fp_ring **q_from_router, uint64_t *masks,
			uint16_t n_neighbors);
	/**
	 * Prepares this driver to run on a specific core.
	 */
	void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat, uint16_t core_index);

	void step();
	void cleanup();

	struct queue_bank_stats *get_queue_bank_stats();
	struct port_drop_stats *get_port_drop_stats();
private:
	Router *m_router;
	struct fp_ring *m_q_to_router; /* must free incoming ring from network */
	struct fp_ring *m_q_from_router[EMU_MAX_OUTPUTS_PER_RTR];
	uint64_t m_port_masks[EMU_MAX_OUTPUTS_PER_RTR]; /* mask for each outgoing queue */
	uint16_t m_neighbors;
	struct emu_admission_core_statistics	*m_stat;
	uint32_t m_random;
	uint16_t m_core_index;
	uint64_t m_cur_time;
};

#endif /* DRIVERS_ROUTERDRIVER_H_ */
