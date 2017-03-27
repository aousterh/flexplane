/*
 * PyScheduler.h
 *
 *  Created on: Dec 19, 2014
 *      Author: yonch
 */

#ifndef SCHEDULERS_PYSCHEDULER_H_
#define SCHEDULERS_PYSCHEDULER_H_

#include "../composite.h"

class PyScheduler: public Scheduler {
public:
	PyScheduler() {};
	virtual ~PyScheduler() {};

	virtual struct emu_packet *schedule(uint32_t output_port,
			uint64_t cur_time) {return NULL;}

	virtual uint64_t *non_empty_port_mask() {return NULL;}
};

#endif /* SCHEDULERS_PYSCHEDULER_H_ */
