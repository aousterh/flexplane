/*
 * benchmark.h
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "benchmark_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#define	ADMITTED_TRAFFIC_MEMPOOL_SIZE		(16*1024)
#define	ADMITTED_TRAFFIC_CACHE_SIZE			512

#define BENCH_MODE_Q_PER_ENQ_CORE	0
#define BENCH_MODE_Q_PER_DEQ_CORE	1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
struct Benchmark;

struct Benchmark *benchmark_get_instance(void);

void benchmark_add_backlog(void *state, uint16_t src, uint16_t dst, uint32_t amount);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#ifdef __cplusplus
#include <vector>
class EnqueueCore;
class DequeueCore;

class Benchmark {
public:
	Benchmark(struct fp_ring *q_admitted_out,
			struct fp_mempool *admitted_traffic_mempool,
			uint32_t n_enqueue_cores, uint32_t mode);

	inline void add_backlog(uint16_t src, uint16_t dst, uint32_t amount);

	/**
	 * Launches a core on the given lcore. Which core is specified in the cmd.
	 */
	void remote_launch_core(void *void_cmd_p, unsigned lcore);

	/**
	 * Execute a specific core.
	 */
	int exec_core(uint32_t core_index);

	/**
	 * Get a pointer to the stats for a specific core.
	 */
	struct benchmark_core_stats *get_core_stats(uint32_t core_index) {
		return m_core_stats[core_index];
	}

	/**
	 * Get a pointer to the global stats.
	 */
	struct benchmark_core_stats *get_global_stats() {
		return &m_stats;
	}

private:
	uint32_t						m_n_enqueue_cores;
	struct fp_mempool				*m_packet_mempool;
	std::vector<struct fp_ring *>	m_enqueue_rings; /* rings to enqueue cores */
	std::vector<struct benchmark_core_stats *>	m_core_stats; /* stats of enq/deq cores */
	struct benchmark_core_stats		m_stats; /* stats for stress test core */

	std::vector<EnqueueCore *>		m_enqueue_cores;
	DequeueCore						*m_dequeue_core;
};
#endif /* __cplusplus */

#endif /* BENCHMARK_H */
