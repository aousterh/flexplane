/*
 * RRScheduler.h
 *
 *  Created on: Dec 29, 2014
 *      Author: hari
 */

#ifndef SCHEDULERS_RRSCHEDULER_H_
#define SCHEDULERS_RRSCHEDULER_H_

#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/random.h"
#include <stdexcept>

class RRScheduler : public Scheduler {
public:
	RRScheduler(PacketQueueBank *bank);
	inline struct emu_packet *schedule(uint32_t port);
	inline uint64_t *non_empty_port_mask();
private:
	PacketQueueBank *m_bank;
	uint64_t        m_last_sched_index;
	uint32_t        random_state; /* first scheduled q (whether or not backlogged; we'll sched first backlogged one following */
};

inline RRScheduler::RRScheduler(PacketQueueBank* bank)
	: m_bank(bank)
{
    seed_random(&random_state, time(NULL));
    m_last_sched_index = random_int(&random_state, sizeof(*non_empty_port_mask)*8); 
}

inline struct emu_packet* __attribute__((always_inline))
RRScheduler::schedule(uint32_t port)
{
	/* get the non empty queue mask for the port */
	uint64_t q_mask = m_bank->non_empty_queue_mask(port);
	uint64_t q_mask_rot = (  (q_mask >> (m_last_sched_index + 1))
						   | (q_mask << (64 - (m_last_sched_index + 1))));
	uint64_t q_index;

	asm("bsfq %1,%0" : "=r"(q_index) : "r"(q_mask_rot));
	m_last_sched_index = (m_last_sched_index + 1 + q_index) % 64;
	return m_bank->dequeue(port, m_last_sched_index);
}

inline uint64_t* RRScheduler::non_empty_port_mask() {
	return m_bank->non_empty_port_mask();
}

#endif /* SCHEDULERS_RRSCHEDULER_H_ */
