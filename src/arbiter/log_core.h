/*
 * log_core.h
 *
 *  Created on: Jan 6, 2014
 *      Author: yonch
 */

#ifndef LOG_CORE_H_
#define LOG_CORE_H_

#include <rte_lcore.h>

struct logged_lcores {
	uint32_t n;
	uint8_t lcore_id[RTE_MAX_LCORE];
};

/* Specifications for controller thread */
struct log_core_cmd {
	uint64_t start_time;
	uint64_t end_time;

	uint64_t log_gap_ticks;
	uint64_t q_log_gap_ticks;

	uint8_t comm_lcore;

	struct logged_lcores comm;
	struct logged_lcores admission;
};

/**
 * Runs the log core
 */
int exec_log_core(void *void_cmd_p);

#endif /* LOG_CORE_H_ */
