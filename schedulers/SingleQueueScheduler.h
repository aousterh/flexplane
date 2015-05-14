/*
 * SingleQueueScheduler.h
 *
 *  Created on: December 18, 2014
 *      Author: yonch
 */

#ifndef SCHEDULERS_SINGLEQUEUESCHEDULER_H_
#define SCHEDULERS_SINGLEQUEUESCHEDULER_H_

#include "composite.h"
#include "queue_bank.h"

#include <stdexcept>

/**
 * Schedules packets by dequeuing one packet from each port. Each output port
 *     must have only a single queue.
 */
class SingleQueueScheduler : public Scheduler {
public:
	SingleQueueScheduler(PacketQueueBank *bank) : m_bank(bank) {}

	inline struct emu_packet *schedule(uint32_t output_port) {
		if (unlikely(m_bank->empty(output_port, 0)))
			throw std::runtime_error("called schedule on an empty port");
		else
			return m_bank->dequeue(output_port, 0);
	}

	inline uint64_t *non_empty_port_mask() {
		return m_bank->non_empty_port_mask();
	}

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;
};

#endif /* SCHEDULERS_SINGLEQUEUESCHEDULER_H_ */
