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
	RRScheduler(PacketQueueBank *bank, uint32_t n_queues_total);
	inline struct emu_packet *schedule(uint32_t port, uint64_t cur_time,
			Dropper *dropper);
	inline uint64_t *non_empty_port_mask();
private:
	PacketQueueBank *m_bank;
	std::vector<uint64_t> m_last_sched_index; /* last sched index per port */
	uint32_t        random_state; /* first scheduled q (whether or not backlogged; we'll sched first backlogged one following */
};

inline RRScheduler::RRScheduler(PacketQueueBank* bank, uint32_t n_ports)
	: m_bank(bank)
{
	uint32_t i;
    seed_random(&random_state, time(NULL));

    for (i = 0; i < n_ports; i++)
    	m_last_sched_index.push_back(random_int(&random_state, 64));
}

inline struct emu_packet* __attribute__((always_inline))
RRScheduler::schedule(uint32_t port, uint64_t cur_time, Dropper *dropper)
{
	/* get the non empty queue mask for the port */
	uint64_t q_mask = m_bank->non_empty_queue_mask(port);
	uint64_t q_mask_rot = (  (q_mask >> (m_last_sched_index[port] + 1))
						   | (q_mask << (64 - (m_last_sched_index[port] + 1))));
	uint64_t q_index;

	asm("bsfq %1,%0" : "=r"(q_index) : "r"(q_mask_rot));
	m_last_sched_index[port] = (m_last_sched_index[port] + 1 + q_index) % 64;
	return m_bank->dequeue(port, m_last_sched_index[port], cur_time);
}

inline uint64_t* RRScheduler::non_empty_port_mask() {
	return m_bank->non_empty_port_mask();
}

#endif /* SCHEDULERS_RRSCHEDULER_H_ */
