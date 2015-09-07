/*
 * benchmark_core.cc
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#include "control.h"
#include "benchmark_core.h"
#include "benchmark_log.h"

#define MAX_PACKET_BURST	128

EnqueueCore::EnqueueCore(struct fp_ring *input_ring,
		struct fp_ring *output_ring)
	: m_input_ring(input_ring), m_output_ring(output_ring) {

	/* initialize log to zeroes */
	memset(&m_stats, 0, sizeof(struct benchmark_core_stats));
}

void EnqueueCore::exec() {
	struct emu_packet *packets[MAX_PACKET_BURST];
	uint32_t counter = 0;
	uint32_t i, n_pkts;
	uint64_t tslot_now, logical_tslot;

	tslot_now = (fp_get_time_ns() * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
	logical_tslot = tslot_now;

	while (1) {
		/* wait until it is time to begin the next timeslot */
		while (tslot_now < logical_tslot) {
			tslot_now = (fp_get_time_ns() * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
			bench_log_core_ahead(&m_stats);
		}

		n_pkts = fp_ring_dequeue_burst(m_input_ring, (void **) &packets[0],
				MAX_PACKET_BURST);
		if (n_pkts == 0) {
			bench_log_packet_dequeue_wait(&m_stats);
		} else {
			/* read all the packets so the core has to pull them into its cache */
			for (i = 0; i < n_pkts; i++)
				counter += packets[i]->flow;

			while (fp_ring_enqueue_bulk(m_output_ring, (void **) &packets[0],
					n_pkts) == -ENOBUFS)
				bench_log_packet_enqueue_wait(&m_stats);
		}

		logical_tslot++;
		bench_log_current_tslot(&m_stats, logical_tslot);
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
	struct emu_packet *packets[MAX_PACKET_BURST];
	uint32_t i, j, n_pkts;
	uint64_t tslot_now, logical_tslot;

	tslot_now = (fp_get_time_ns() * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
	logical_tslot = tslot_now;

	while (1) {
		/* wait until it is time to begin the next timeslot */
		while (tslot_now < logical_tslot) {
			tslot_now = (fp_get_time_ns() * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
			bench_log_core_ahead(&m_stats);
		}

		/* iterate over all input rings */
		for (i = 0; i < m_input_rings.size(); i++) {
			n_pkts = fp_ring_dequeue_burst(m_input_rings[i],
					(void **) &packets[0], MAX_PACKET_BURST);
			if (n_pkts == 0) {
				bench_log_packet_dequeue_wait(&m_stats);
				continue; /* nothing to dequeue from this ring right now */
			}

			/* got a batch of packets */

			/* add packets' data to admitted struct */
			for (j = 0; j < n_pkts; j++) {
				admitted_insert_admitted_edge(m_admitted, packets[j]);

				if (m_admitted->size == EMU_ADMITS_PER_ADMITTED)
					flush();
			}

			/* free the packets */
			fp_mempool_put_bulk(m_packet_mempool, (void **) &packets[0],
					n_pkts);
		}

		logical_tslot++;
		bench_log_current_tslot(&m_stats, logical_tslot);
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
