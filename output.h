/*
 * output.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "admitted.h"
#include "packet.h"

/**
 * A class used to admit and drop packets
 */
class EmulationOutput {
public:
	/**
	 * c'tor
	 * @param q_admitted: ring where admitted batches are enqueued
	 * @param admitted_mempool: a mempool for allocation of admitted batches
	 * @param packet_mempool: a mempool to free packets.
	 */
	EmulationOutput(struct fp_ring *q_admitted,
			struct fp_mempool *admitted_mempool,
			struct fp_mempool *packet_mempool,
			struct emu_admission_statistics	*stat);

	/**
	 * d'tor
	 */
	~EmulationOutput();

	/**
	 * Notifies physical endpoint to drop packet and frees packet memory.
	 * @param packet: packet to be dropped
	 */
	inline void drop(struct emu_packet *packet);

	/**
	 * Admites the given packet
	 * @param packet: packet to be admitted
	 */
	inline void admit(struct emu_packet *packet);

	/**
	 * flushes a batch of admitted and dropped packets to q_admitted
	 */
	inline void flush();

	/**
	 * Drops a raw demand
	 *
	 * This method is needed for the system to gracefully handle running out of
	 *   memory in packet_mempool. In that case, packets are dropped without
	 *   the need for a struct emu_packet *
	 */
	inline void drop_raw(uint16_t src, uint16_t dst, uint16_t flow);

private:
	/** frees the given packet into packet_mempool */
	inline void free_packet(struct emu_packet *packet);

	/** ring to enqueue admitted batches */
	struct fp_ring					*q_admitted_out;

	/** mempool to allocate admitted batches */
	struct fp_mempool				*admitted_traffic_mempool;

	/** mempool to free packets */
	struct fp_mempool				*packet_mempool;

	/** statistics */
	struct emu_admission_statistics	*stat;

	/** the next batch to be flushed */
	struct emu_admitted_traffic		*admitted;
};

/**
 * A class that can be used for dropping packets
 */
class Dropper : private EmulationOutput {
public:
	Dropper(struct fp_ring *q_admitted,
			struct fp_mempool *admitted_mempool,
			struct fp_mempool *packet_mempool,
			struct emu_admission_statistics	*stat)
		: EmulationOutput(q_admitted, admitted_mempool, packet_mempool, stat) {}

	using EmulationOutput::drop;
};

/** implementation */

inline
EmulationOutput::EmulationOutput(struct fp_ring* q_admitted,
		struct fp_mempool* admitted_mempool,
		struct fp_mempool* _packet_mempool,
		struct emu_admission_statistics	*_stat)
	: q_admitted_out(q_admitted),
	  admitted_traffic_mempool(admitted_mempool),
	  packet_mempool(_packet_mempool),
	  stat(_stat)
{
	/* allocate the next batch to be flushed */
	while (fp_mempool_get(admitted_traffic_mempool,
			(void **) &admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(stat);

	admitted_init(admitted);
}

inline EmulationOutput::~EmulationOutput() {
	/* free the allocated next batch */
	fp_mempool_put(admitted_traffic_mempool, admitted);
}

inline void __attribute__((always_inline))
EmulationOutput::drop(struct emu_packet* packet)
{
	drop_raw(packet->src, packet->dst, packet->flow);

	free_packet(packet);

	adm_log_emu_dropped_packet(stat);
}

inline void __attribute__((always_inline))
EmulationOutput::admit(struct emu_packet* packet)
{
	admitted_insert_admitted_edge(admitted, packet->src, packet->dst,
			packet->flow);
	adm_log_emu_admitted_packet(stat);

	free_packet(packet);
}

inline void EmulationOutput::drop_raw(uint16_t src, uint16_t dst,
		uint16_t flow)
{
	admitted_insert_dropped_edge(admitted, src, dst, flow);
	adm_log_emu_dropped_demand(stat);
}

inline void __attribute__((always_inline))
EmulationOutput::flush()
{
	/* send out the admitted traffic */
	while (fp_ring_enqueue(q_admitted_out, admitted) != 0)
		adm_log_emu_wait_for_admitted_enqueue(stat);

	/* get 1 new admitted traffic, init it */
	while (fp_mempool_get(admitted_traffic_mempool,
				(void **) &admitted) == -ENOENT)
		adm_log_emu_admitted_alloc_failed(stat);

	admitted_init(admitted);
}

inline void EmulationOutput::free_packet(struct emu_packet* packet)
{
	/* return the packet to the mempool */
	fp_mempool_put(packet_mempool, packet);
}

#endif /* OUTPUT_H_ */
