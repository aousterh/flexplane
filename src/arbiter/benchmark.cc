/*
 * benchmark.cc
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#include "benchmark.h"
#include "benchmark_core.h"
#include "../emulation/packet.h"
#include "../emulation/packet_impl.h"
#include "../emulation/util/make_mempool.h"
#include "../emulation/util/make_ring.h"

#define BENCH_QUEUE_LOG_SIZE				16
#define BENCH_PACKET_MEMPOOL_SIZE			(512 * 1024)
#define	BENCH_PACKET_MEMPOOL_CACHE_SIZE		256

Benchmark *g_benchmark;

Benchmark *benchmark_get_instance(void)
{
	return g_benchmark;
}

void benchmark_add_backlog(void *state, uint16_t src, uint16_t dst,
		uint32_t amount) {
	Benchmark *benchmark = (Benchmark *) state;
	benchmark->add_backlog(src, dst, amount);
}

Benchmark::Benchmark(struct rte_ring *q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool, uint32_t n_enqueue_cores)
	: m_n_enqueue_cores(n_enqueue_cores)
{
	uint32_t i;
	char s[64];
	fp_ring *ring;
	uint32_t packet_ring_size;
	struct fp_ring *dequeue_ring;

	/* create packet mempool */
	m_packet_mempool = make_mempool("packet_mempool",
			BENCH_PACKET_MEMPOOL_SIZE, EMU_ALIGN(sizeof(struct emu_packet)),
			BENCH_PACKET_MEMPOOL_CACHE_SIZE, 0, 0);

	/* initialize all packet rings */
	packet_ring_size = (1 << BENCH_QUEUE_LOG_SIZE);
	for (i = 0; i < n_enqueue_cores; i++) {
		snprintf(s, sizeof(s), "enqueue_q_%d", i);
		ring = make_ring(s, packet_ring_size, 0,
				RING_F_SP_ENQ | RING_F_SC_DEQ);
		m_enqueue_rings.push_back(ring);
	}
	snprintf(s, sizeof(s), "dequeue_q_%d", i);
	dequeue_ring = make_ring(s, packet_ring_size, 0,
			RING_F_SP_ENQ | RING_F_SC_DEQ);

	/* construct all cores */
	for (i = 0; i < n_enqueue_cores; i++) {
		EnqueueCore *core = new EnqueueCore(m_enqueue_rings[i], dequeue_ring);
		m_enqueue_cores.push_back(core);
		m_core_stats.push_back(core->get_stats());
	}
	m_dequeue_core = new DequeueCore(dequeue_ring, m_packet_mempool,
			q_admitted_out, admitted_traffic_mempool);
	m_core_stats.push_back(m_dequeue_core->get_stats());

	/* initialize log to zeroes */
	memset(&m_stats, 0, sizeof(struct benchmark_core_stats));

	/* set global benchmark */
	g_benchmark = this;
}

inline void Benchmark::add_backlog(uint16_t src, uint16_t dst, uint32_t amount)
{
	uint32_t i;
	struct emu_packet *packet;

	for (i = 0; i < amount; i++) {
		while (fp_mempool_get(m_packet_mempool, (void **) &packet) == -ENOENT)
			bench_log_mempool_get_wait(&m_stats);

		/* fill flow with random field */
		packet_init(packet, src, dst, rand(), 0, NULL);

		while (fp_ring_enqueue(m_enqueue_rings[src], (void *) packet) ==
				-ENOBUFS)
			bench_log_packet_enqueue_wait(&m_stats);
	}
}

static int exec_benchmark_core(void *void_cmd_p) {
	struct benchmark_core_cmd *cmd = (struct benchmark_core_cmd *) void_cmd_p;
	uint32_t core_index = cmd->core_index;
	Benchmark *benchmark = cmd->benchmark;

	return benchmark->exec_core(core_index);
}

void Benchmark::remote_launch_core(void *void_cmd_p, unsigned lcore) {

	rte_eal_remote_launch(exec_benchmark_core, void_cmd_p, lcore);
}

int Benchmark::exec_core(uint32_t core_index) {
	/* run the appropriate core */
	if (core_index < m_n_enqueue_cores)
		m_enqueue_cores[core_index]->exec();
	else
		m_dequeue_core->exec();

	return 0;
}
