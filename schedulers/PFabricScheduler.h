/*
 * PFabricScheduler.h
 *
 *  Created on: September 6, 2015
 *      Author: aousterh
 */

#ifndef SCHEDULERS_PFABRICSCHEDULER_H_
#define SCHEDULERS_PFABRICSCHEDULER_H_

#include "composite.h"
#include "pfabric_queue_bank.h"

#include <stdexcept>

/**
 * Schedules packets by dequeuing the lowest priority packet from each port.
 */
class PFabricScheduler : public Scheduler {
public:
	SingleQueueScheduler(PFabricQueueBank *bank) : m_bank(bank) {}

	inline struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time,
			Dropper *dropper)
	{
		if (unlikely(m_bank->empty(output_port)))
			throw std::runtime_error("called schedule on an empty port");
		else
			return m_bank->dequeue_lowest_priority(output_port);
	}

	inline uint64_t *non_empty_port_mask() {
		return m_bank->non_empty_port_mask();
	}

protected:
	/** the QueueBank where packets are stored */
	PFabricQueueBank *m_bank;
};

#endif /* SCHEDULERS_PFABRICSCHEDULER_H_ */
