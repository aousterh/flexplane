/*
 * PriorityScheduler.h
 *
 *  Created on: Dec 26, 2014
 *      Author: yonch
 */

#ifndef SCHEDULERS_PRIORITYSCHEDULER_H_
#define SCHEDULERS_PRIORITYSCHEDULER_H_

#include "composite.h"
#include "queue_bank.h"

#include <stdexcept>

class PriorityScheduler : public Scheduler {
public:
	PriorityScheduler(PacketQueueBank *bank);
	inline struct emu_packet *schedule(uint32_t port);
	inline uint64_t *non_empty_port_mask();
private:
	PacketQueueBank *m_bank;
};

inline PriorityScheduler::PriorityScheduler(PacketQueueBank* bank)
	: m_bank(bank)
{}

inline struct emu_packet* __attribute__((always_inline))
PriorityScheduler::schedule(uint32_t port)
{
	/* get the non empty queue mask for the port */
	uint64_t q_mask = m_bank->non_empty_queue_mask(port);

	uint64_t q_index;
	asm("bsfq %1,%0" : "=r"(q_index) : "r"(q_mask));

	return m_bank->dequeue(port, q_index);
}

inline uint64_t* PriorityScheduler::non_empty_port_mask() {
	return m_bank->non_empty_port_mask();
}

#endif /* SCHEDULERS_PRIORITYSCHEDULER_H_ */
