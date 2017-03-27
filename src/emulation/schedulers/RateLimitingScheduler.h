/*
 * RateLimitingScheduler.h
 *
 *  Created on: June 30, 2015
 *      Author: aousterh
 */

#ifndef SCHEDULERS_RATELIMITINGSCHEDULER_H_
#define SCHEDULERS_RATELIMITINGSCHEDULER_H_

#include "composite.h"
#include "queue_bank.h"

#include <stdexcept>

/**
 * Schedules packets at a fixed rate. This is a very primitive design: after
 * sending a packet, it will not allow another packet to be sent until exactly
 * t_btwn_pkts timeslots have elapsed.
 */
class RateLimitingScheduler : public SingleQueueScheduler {
public:
	RateLimitingScheduler(PacketQueueBank *bank, uint32_t n_ports,
			uint16_t t_btwn_pkts)
		: SingleQueueScheduler(bank),
		  m_bank(bank),
		  m_t_btwn_pkts(t_btwn_pkts)
	{
		uint32_t i;

		/* set all last send times to 0 */
		m_last_send_times.reserve(n_ports);
		for (i = 0; i < n_ports; i++)
			m_last_send_times.push_back(0);
	}

	inline struct emu_packet *schedule(uint32_t output_port,
			uint64_t cur_time, Dropper *dropper);

protected:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/* how many timeslots between consecutive packets being scheduled */
	uint16_t m_t_btwn_pkts;

	/* last time a packet was sent, for each port */
	std::vector<uint64_t> m_last_send_times;
};

inline struct emu_packet *RateLimitingScheduler::schedule(uint32_t output_port,
		uint64_t cur_time, Dropper *dropper)
{
	struct emu_packet *pkt;

	if (cur_time - m_last_send_times[output_port] < m_t_btwn_pkts)
		return NULL; /* can't send yet */

	/* call parent to dequeue packet from single queue */
	pkt = SingleQueueScheduler::schedule(output_port, cur_time, dropper);
	m_last_send_times[output_port] = cur_time;

	return pkt;
}

#endif /* SCHEDULERS_RATELIMITINGSCHEDULER_H_ */
