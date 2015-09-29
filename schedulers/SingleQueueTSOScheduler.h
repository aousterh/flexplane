/*
 * SingleQueueTSOScheduler.h
 *
 *  Created on: September 25, 2015
 *      Author: aousterh
 */

#ifndef SCHEDULERS_SINGLEQUEUETSOSCHEDULER_H_
#define SCHEDULERS_SINGLEQUEUETSOSCHEDULER_H_

#include "composite.h"
#include "queue_bank.h"

#include <stdexcept>

/**
 * Schedules packets by dequeuing one packet from each port, handling large TSO
 * 	segments. Each output port must have only a single queue.
 */
class SingleQueueTSOScheduler : public Scheduler {
public:
	SingleQueueTSOScheduler(PacketQueueBank *bank, uint32_t n_ports)
		: m_bank(bank) {
		uint16_t i;

		for (i = 0; i < n_ports; i++)
			m_wait_time.push_back(0);
	}

	inline struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time,
			Dropper *dropper)
	{
		if (unlikely(m_bank->empty(output_port, 0)))
			throw std::runtime_error("called schedule on an empty port");

		struct emu_packet *p;

		if (m_wait_time[output_port] > 0)
			m_wait_time[output_port]--;

		if (m_wait_time[output_port] > 0) {
			return NULL; /* can't send yet */
		}
		else {
			p = m_bank->dequeue(output_port, 0, cur_time);
			m_wait_time[output_port] += p->n_mtus;
			m_bank->decrement_tso_occupancy(output_port, 0, p->n_mtus);
			return p;
		}
	}

	inline uint64_t *non_empty_port_mask() {
		return m_bank->non_empty_port_mask();
	}

protected:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** number of timeslots until next send time, per port */
	std::vector<uint32_t> m_wait_time;
};

#endif /* SCHEDULERS_SINGLEQUEUETSOSCHEDULER_H_ */
