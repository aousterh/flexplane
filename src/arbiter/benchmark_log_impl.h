/*
 * benchmark_log_impl.h
 *
 *  Created on: August 7, 2015
 *      Author: aousterh
 */

#ifndef BENCHMARK_LOG_IMPL_H
#define BENCHMARK_LOG_IMPL_H

#include "benchmark_log.h"
#include "control.h"

struct benchmark_core_stats bench_saved_core_stats[N_BENCHMARK_CORES];
struct benchmark_core_stats bench_saved_stat;

/**
 * Print benchmark core log, where st is the current stat and sv is the last
 * saved stat.
 */
void print_benchmark_core_log(struct benchmark_core_stats *st,
		struct benchmark_core_stats *sv) {
#define D(X) (st->X - sv->X)
	printf("\n  waits: %lu packet deq, %lu packet enq, ",
			D(packet_dequeue_wait), D(packet_enqueue_wait));
	printf("%lu mempool get, %lu admitted enqueue", D(mempool_get_wait),
			D(admitted_enqueue_wait));
	printf("\n  drops: %lu on failed enqueue, ahead: %lu, current: %lu",
			D(packet_drop_on_failed_enqueue), D(ahead), D(current_tslot));
#undef D

	printf("\n  waits: %lu packet deq, %lu packet enq, ",
			st->packet_dequeue_wait, st->packet_enqueue_wait);
	printf("%lu mempool get, %lu admitted enqueue", st->mempool_get_wait,
			st->admitted_enqueue_wait);
	printf("\n  drops: %lu on failed enqueue, ahead: %lu, current: %lu",
			st->packet_drop_on_failed_enqueue, st->ahead, st->current_tslot);
	printf("\n");
}

void print_and_save_benchmark_core_log(uint32_t core_index) {
	Benchmark *benchmark = benchmark_get_instance();
	struct benchmark_core_stats *st = benchmark->get_core_stats(core_index);
	struct benchmark_core_stats *sv = &bench_saved_core_stats[core_index];

	printf("\nbenchmark core %d", core_index);

	print_benchmark_core_log(st, sv);

	/* save current stats */
	memcpy(&bench_saved_core_stats[core_index], st,
			sizeof(struct benchmark_core_stats));
}

void print_and_save_global_benchmark_log() {
	Benchmark *benchmark = benchmark_get_instance();
	struct benchmark_core_stats *st = benchmark->get_global_stats();
	struct benchmark_core_stats *sv = &bench_saved_stat;

	printf("\nglobal benchmark");

	print_benchmark_core_log(st, sv);

	memcpy(&bench_saved_stat, st, sizeof(struct benchmark_core_stats));
}

#endif /* BENCHMARK_LOG_IMPL_H */
