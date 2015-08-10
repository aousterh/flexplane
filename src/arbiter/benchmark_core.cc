/*
 * benchmark_core.cc
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#include "benchmark_core.h"
#include "benchmark_log.h"

EnqueueCore::EnqueueCore(struct fp_ring *input_ring,
		struct fp_ring *output_ring)
	: m_input_ring(input_ring), m_output_ring(output_ring) {

	/* initialize log to zeroes */
	memset(&m_stats, 0, sizeof(struct benchmark_core_stats));
}

void EnqueueCore::exec() {
	struct emu_packet *packet;
	uint32_t counter;

	while (1) {
		while (fp_ring_dequeue(m_input_ring, (void **) &packet) == -ENOENT)
			bench_log_packet_dequeue_wait(&m_stats);

		/* read the packet so the core has to pull it into its cache */
		counter += packet->flow;

		while (fp_ring_enqueue(m_output_ring, packet) == -ENOBUFS)
			bench_log_packet_enqueue_wait(&m_stats);
	}
}

DequeueCore::DequeueCore(struct fp_ring **input_rings, uint32_t n_input_rings,
		struct fp_mempool *packet_mempool, struct fp_ring *q_admitted_out,
		struct fp_mempool *admitted_mempool)
	: m_packet_mempool(packet_mempool),
	  m_q_admitted_out(q_admitted_out),
	  m_admitted_mempool(admitted_mempool)
{
	uint32_t i;

	/* initialize log to zeroes */
	memset(&m_stats, 0, sizeof(struct benchmark_core_stats));

	/* get the first admitted traffic */
	while (fp_mempool_get(admitted_mempool, (void **) &m_admitted) == -ENOENT)
		bench_log_mempool_get_wait(&m_stats);

	/* copy pointers to input rings */
	for (i = 0; i < n_input_rings; i++)
		m_input_rings.push_back(input_rings[i]);
}

void DequeueCore::exec() {
	struct emu_packet *packet;
	uint32_t i;

	while (1) {
		/* iterate over all input rings */
		for (i = 0; i < m_input_rings.size(); i++) {
			if (fp_ring_dequeue(m_input_rings[i], (void **) &packet) ==
					-ENOENT) {
				bench_log_packet_dequeue_wait(&m_stats);
				continue; /* nothing to dequeue from this ring right now */
			}

			/* got a packet */

			/* add packet's data to admitted struct */
			admitted_insert_admitted_edge(m_admitted, packet, NULL);
			if (m_admitted->size == EMU_ADMITS_PER_ADMITTED)
				flush();

			/* free the packet */
			fp_mempool_put(m_packet_mempool, packet);
		}
	}
}

void DequeueCore::flush()
{
	/* send out the admitted traffic */
	while (fp_ring_enqueue(m_q_admitted_out, m_admitted) != 0)
		bench_log_admitted_enqueue_wait(&m_stats);

	/* get a batch of new admitted traffic, init it */
	while (fp_mempool_get(m_admitted_mempool, (void **) &m_admitted) ==
			-ENOENT)
		bench_log_mempool_get_wait(&m_stats);

	admitted_init(m_admitted);
}
