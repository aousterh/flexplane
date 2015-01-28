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
#include "queue_bank_log.h"

/* pull admitted structs in batches to avoid contention */
#define N_ADMITTEDS_PER_OUTPUT	64

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
	 * @param stat: stats for this core
	 * @param start_n_admitted: number of admitted structs to start with
	 */
	EmulationOutput(struct fp_ring *q_admitted,
			struct fp_mempool *admitted_mempool,
			struct fp_mempool *packet_mempool,
			struct emu_admission_core_statistics *stat,
			uint16_t start_n_admitted);

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
	 * Admits the given packet
	 * @param packet: packet to be admitted
	 */
	inline void admit(struct emu_packet *packet);

	/**
	 * Flushes a batch of admitted and dropped packets to q_admitted
	 */
	inline void flush();

	/**
	 * Frees the given packet into packet_mempool
	 *
	 * @important: don't free a packet that the emulation framework is expecting
	 *    to either be admitted or dropped -- drop() or admit() it instead.
	 * @note: this function aides resetting an endpoints' queues when the
	 *    endpoint resets the connection to the emulation
	 */
	inline void free_packet(struct emu_packet *packet);

private:
	/** ring to enqueue admitted batches */
	struct fp_ring					*q_admitted_out;

	/** mempool to allocate admitted batches */
	struct fp_mempool				*admitted_traffic_mempool;

	/** mempool to free packets */
	struct fp_mempool				*packet_mempool;

	/** statistics */
	struct emu_admission_core_statistics	*m_stat;

	/** the next batch to be flushed */
	uint16_t						m_admitted_index;
	struct emu_admitted_traffic		*admitted[N_ADMITTEDS_PER_OUTPUT];
};

/**
 * A class that can be used for dropping packets
 */
class Dropper {
public:
	Dropper(EmulationOutput &emu_output, struct queue_bank_stats *stats)
		: m_emu_output(emu_output), m_stats(stats) {}

	inline void  __attribute__((always_inline))	drop(struct emu_packet *packet,
			uint32_t port) {
		m_emu_output.drop(packet);
		queue_bank_log_drop(m_stats, port);
	}

private:
	EmulationOutput &m_emu_output;
	struct queue_bank_stats *m_stats;
};

/** implementation */

inline
EmulationOutput::EmulationOutput(struct fp_ring* q_admitted,
		struct fp_mempool* admitted_mempool,
		struct fp_mempool* _packet_mempool,
		struct emu_admission_core_statistics *_stat, uint16_t start_n_admitted)
	: q_admitted_out(q_admitted),
	  admitted_traffic_mempool(admitted_mempool),
	  packet_mempool(_packet_mempool),
	  m_stat(_stat)
{
	/* allocate the next batch to be flushed */
	while (fp_mempool_get_bulk(admitted_traffic_mempool,
			(void **) &admitted[0], start_n_admitted) == -ENOENT) {
		printf("loop\n");
		adm_log_emu_admitted_alloc_failed(m_stat);
	}

	m_admitted_index = start_n_admitted - 1;
	admitted_init(admitted[m_admitted_index]);
}

inline EmulationOutput::~EmulationOutput() {
	uint16_t i;

	/* free the allocated next batch */
	for (i = 0; i <= m_admitted_index; i++)
		fp_mempool_put(admitted_traffic_mempool, admitted[i]);
}

inline void __attribute__((always_inline))
EmulationOutput::drop(struct emu_packet* packet)
{
	/* add dropped packet to admitted struct */
	admitted_insert_dropped_edge(admitted[m_admitted_index], packet, m_stat);
	adm_log_emu_dropped_packet(m_stat);

	/* if admitted struct is full, flush now */
	if (unlikely(admitted[m_admitted_index]->size == EMU_ADMITS_PER_ADMITTED))
		flush();

	free_packet(packet);
}

inline void __attribute__((always_inline))
EmulationOutput::admit(struct emu_packet* packet)
{
	admitted_insert_admitted_edge(admitted[m_admitted_index], packet, m_stat);
	adm_log_emu_admitted_packet(m_stat);

	/* if admitted struct is full, flush now */
	if (unlikely(admitted[m_admitted_index]->size == EMU_ADMITS_PER_ADMITTED))
		flush();

	free_packet(packet);
}

inline void __attribute__((always_inline))
EmulationOutput::flush()
{
	/* send out the admitted traffic */
	while (fp_ring_enqueue(q_admitted_out, admitted[m_admitted_index]) != 0)
		adm_log_emu_wait_for_admitted_enqueue(m_stat);

	/* get a batch of new admitted traffic, init it */
	if (m_admitted_index == 0) {
		while (fp_mempool_get_bulk(admitted_traffic_mempool,
				(void **) &admitted[0], N_ADMITTEDS_PER_OUTPUT) == -ENOENT)
			adm_log_emu_admitted_alloc_failed(m_stat);
		m_admitted_index = N_ADMITTEDS_PER_OUTPUT - 1;
	} else {
		m_admitted_index--;
	}

	admitted_init(admitted[m_admitted_index]);
}

inline void EmulationOutput::free_packet(struct emu_packet* packet)
{
	/* return the packet to the mempool */
	fp_mempool_put(packet_mempool, packet);
}

#endif /* OUTPUT_H_ */
