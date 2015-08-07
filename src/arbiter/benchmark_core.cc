/*
 * benchmark_core.cc
 *
 *  Created on: August 6, 2015
 *      Author: aousterh
 */

#include "benchmark_core.h"

EnqueueCore::EnqueueCore(struct fp_ring *input_ring,
		struct fp_ring *output_ring)
	: m_input_ring(input_ring), m_output_ring(output_ring)
{}

void EnqueueCore::exec() {
	struct emu_packet *packet;
	uint32_t counter;

	while (1) {
		while (fp_ring_dequeue(m_input_ring, (void **) &packet) == -ENOENT) {}

		/* read the packet so the core has to pull it into its cache */
		counter += packet->flow;

		while (fp_ring_enqueue(m_output_ring, packet) == -ENOBUFS) {}
	}
}

DequeueCore::DequeueCore(struct fp_ring *input_ring,
		struct fp_mempool *packet_mempool, struct fp_ring *q_admitted_out,
		struct fp_mempool *admitted_mempool)
	: m_input_ring(input_ring),
	  m_packet_mempool(packet_mempool),
	  m_q_admitted_out(q_admitted_out),
	  m_admitted_mempool(admitted_mempool)
{
	while (fp_mempool_get(admitted_mempool, (void **) &m_admitted) == -ENOENT) {}
}

void DequeueCore::exec() {
	struct emu_packet *packet;

	while (1) {
		while (fp_ring_dequeue(m_input_ring, (void **) &packet) == -ENOENT) {}

		/* add packet's data to admitted struct */
		admitted_insert_admitted_edge(m_admitted, packet, NULL);
		if (m_admitted->size == EMU_ADMITS_PER_ADMITTED)
			flush();

		/* free the packet */
		fp_mempool_put(m_packet_mempool, packet);
	}
}

void DequeueCore::flush()
{
	/* send out the admitted traffic */
	while (fp_ring_enqueue(m_q_admitted_out, m_admitted) != 0) {}

	/* get a batch of new admitted traffic, init it */
	while (fp_mempool_get(m_admitted_mempool, (void **) &m_admitted) ==
			-ENOENT) {}

	admitted_init(m_admitted);
}
