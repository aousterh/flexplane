/*
 * log_core.h
 *
 *  Created on: Jan 6, 2014
 *      Author: yonch
 */

#ifndef LOG_CORE_H_
#define LOG_CORE_H_

#include <rte_lcore.h>
#include <vector>
#include <stdint.h>

struct queue_bank_stats;
struct port_drop_stats;

class LogCore {
public:
	LogCore(uint64_t log_gap_ticks,	uint64_t q_log_gap_ticks);

	/* instruct log core to log these objects */
	void add_comm_lcore(uint8_t lcore);
	void add_logged_lcore(uint8_t lcore);
	void add_queueing_stats(struct queue_bank_stats* queue_stats);
	void add_drop_stats(struct port_drop_stats *port_stats);

	/**
	 * Runs on the current lcore
	 */
	int exec();

	/**
	 * Launches the core on the given lcore
	 */
	void remote_launch(unsigned lcore);

private:
	uint64_t m_log_gap_ticks;
	uint64_t m_q_log_gap_ticks;
	std::vector<uint8_t> m_comm_lcores;
	std::vector<uint8_t> m_logged_lcores; /* lcores with logged stats */
	std::vector<struct queue_bank_stats *> m_queue_stats;
	std::vector<struct port_drop_stats *> m_port_stats;
};

#endif /* LOG_CORE_H_ */
