/*
 * benchmark_log.h
 *
 *  Created on: August 7, 2015
 *      Author: aousterh
 */

#ifndef BENCHMARK_LOG_H
#define BENCHMARK_LOG_H

#include <stdint.h>

#define		MAINTAIN_BENCHMARK_COUNTERS	1

/**
 * Per-core statistics for benchmark
 */
struct benchmark_core_stats {
	uint64_t packet_dequeue_wait;
	uint64_t packet_enqueue_wait;
	uint64_t mempool_get_wait;
	uint64_t admitted_enqueue_wait;
};

static inline __attribute__((always_inline))
void bench_log_packet_dequeue_wait(struct benchmark_core_stats *st) {
	if (MAINTAIN_BENCHMARK_COUNTERS)
		st->packet_dequeue_wait++;
}

static inline __attribute__((always_inline))
void bench_log_packet_enqueue_wait(struct benchmark_core_stats *st) {
	if (MAINTAIN_BENCHMARK_COUNTERS)
		st->packet_enqueue_wait++;
}

static inline __attribute__((always_inline))
void bench_log_mempool_get_wait(struct benchmark_core_stats *st) {
	if (MAINTAIN_BENCHMARK_COUNTERS)
		st->mempool_get_wait++;
}

static inline __attribute__((always_inline))
void bench_log_admitted_enqueue_wait(struct benchmark_core_stats *st) {
	if (MAINTAIN_BENCHMARK_COUNTERS)
		st->admitted_enqueue_wait++;
}

#endif /* BENCHMARK_LOG_H */
