/*
 * benchmark_core.h
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#ifndef BENCHMARK_CORE_H
#define BENCHMARK_CORE_H

#include "benchmark_log.h"
#include "../emulation/admitted.h"
#include "../emulation/packet.h"
#include "../graph-algo/fp_ring.h"

class Benchmark;

/* Specifications for the benchmark thread */
struct benchmark_core_cmd {
	uint32_t core_index;
	Benchmark *benchmark;
};

class EnqueueCore {
public:
	EnqueueCore(struct fp_ring *input_ring, struct fp_ring *output_ring);

	/* run this enqueue core */
	void exec();

	/* return a pointer to the stats for this core */
	struct benchmark_core_stats *get_stats() {
		return &m_stats;
	}

private:
	struct fp_ring	*m_input_ring;
	struct fp_ring	*m_output_ring;
	struct benchmark_core_stats	m_stats;
};

class DequeueCore {
public:
	DequeueCore(struct fp_ring *input_ring, struct fp_mempool *packet_mempool,
			struct fp_ring *q_admitted_out,
			struct fp_mempool *admitted_mempool);

	/* run this enqueue core */
	void exec();

	/* return a pointer to the stats for this core */
	struct benchmark_core_stats *get_stats() {
		return &m_stats;
	}

private:
	/* send out m_admitted and get a new one */
	void flush();

	struct emu_admitted_traffic *m_admitted;

	struct fp_ring		*m_input_ring;
	struct fp_mempool	*m_packet_mempool;
	struct fp_ring		*m_q_admitted_out;
	struct fp_mempool	*m_admitted_mempool;
	struct benchmark_core_stats	m_stats;
};

#endif /* BENCHMARK_CORE_H */
