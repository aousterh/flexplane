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

class LogCore {
public:
	LogCore(uint64_t log_gap_ticks,	uint64_t q_log_gap_ticks);

	/* instruct log core to log these objects */
	void add_comm_lcore(uint8_t lcore);
	void add_admission_lcore(uint8_t lcore);

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
	std::vector<uint8_t> m_admission_lcores;
};

#endif /* LOG_CORE_H_ */
