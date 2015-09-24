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
	SingleQueueScheduler(PacketQueueBank *bank) : m_bank(bank) {
#ifdef USE_TSO
		uint16_t i;
		/* TODO: fix this to be more generl */
		for (i = 0; i < 64; i++)
			m_tso_debt.push_back(0);
#endif
	}

	inline struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time,
			Dropper *dropper)
	{
#ifndef USE_TSO
		if (unlikely(m_bank->empty(output_port, 0)))
			throw std::runtime_error("called schedule on an empty port");
		else
			return m_bank->dequeue(output_port, 0, cur_time);
#else
		struct emu_packet *p;

		if (m_tso_debt[output_port] > 0)
			m_tso_debt[output_port]--;

		if (m_tso_debt[output_port] > 0) {
			return NULL; /* can't send yet */
		}
		else {
			p = m_bank->dequeue(output_port, 0, cur_time);
			m_tso_debt[output_port] += p->n_mtus;
			m_bank->decrement_tso_occupancy(output_port, 0, p->n_mtus);
			return p;
		}
#endif
	}

	inline uint64_t *non_empty_port_mask() {
		return m_bank->non_empty_port_mask();
	}

protected:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	std::vector<uint32_t> m_tso_debt;
};

#endif /* SCHEDULERS_SINGLEQUEUESCHEDULER_H_ */
