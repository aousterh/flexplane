/*
 * queue_bank.h
 *
 *  Created on: Dec 15, 2014
 *      Author: yonch
 */

#ifndef QUEUE_BANK_H_
#define QUEUE_BANK_H_

#include <vector>
#include <stdint.h>
#include <stdexcept>
#include "circular_queue.h"
#include "queue_bank_log.h"
#include "../graph-algo/platform.h"

/**
 * A collection of queues. The queue bank keeps M queues for each of N input
 *   ports
 */
template <typename ELEM >
class QueueBank {
public:
	/**
	 * c'tor
	 *
	 * @param n_ports: the number of ports in the queue bank
	 * @param n_queues: the the number of queues per port
	 * @param queue_max_size: the maximum number of packets each queue can hold
	 * @param stats: enqueue/dequeue stats
	 *
	 * @important: queue_max_size must be a power of two
	 * @important: n_queues must be <= 64
	 */
	QueueBank(uint32_t n_ports, uint32_t n_queues, uint32_t queue_max_size);

	/**
	 * d'tor
	 * @assumes all memory pointed to by queues is already freed
	 */
	~QueueBank();

	/**
	 * Returns the flat index of the given queue
	 */
	inline uint32_t flat_index(uint32_t port, uint32_t queue);

	/**
	 * Enqueues the element
	 * @param queue_index: the flat index of the queue
	 * @param e: the element to enqueue
	 * @assumes there is enough space in q
	 */
	inline void enqueue(uint32_t port, uint32_t queue, ELEM *e);

	/**
	 * Dequeues an element
	 * @param queue_index: the flat index of the queue to dequeue
	 * @assumes the queue is non-empty
	 */
	inline ELEM *dequeue(uint32_t port, uint32_t queue, uint64_t cur_time);

	/**
	 * @return a pointer to a bit mask with 1 for ports with packets, 0 o/w.
	 * @important user must not modify the bitmask contents
	 */
	inline uint64_t *non_empty_port_mask();

	/**
	 * @param port: the port for which to get the empty queue mask
	 * @return a bit mask, with 1 for queues with packets
	 */
	inline uint64_t non_empty_queue_mask(uint32_t port);

	/**
	 * @returns 1 if queue is empty, 0 otherwise
	 */
	inline int empty(uint32_t port, uint32_t queue);

	/**
	 * @returns the number of elements in queue
	 * @param queue_index: the flat index of the queue
	 */
	inline uint32_t occupancy(uint32_t port, uint32_t queue);

	/**
	 * @returns 1 if queue is empty, 0 otherwise
	 */
	inline int full(uint32_t port, uint32_t queue);

	/**
	 * @returns the last time the queue went from non-empty to empty
	 */
	inline int last_empty_time(uint32_t port, uint32_t queue);

	/**
	 * @returns a pointer to the queue bank stats
	 */
	inline struct queue_bank_stats *get_queue_bank_stats();

	/**
	 * Get TSO occupancy count
	 */
	inline uint16_t get_tso_occupancy(uint32_t port, uint32_t queue);

	/**
	 * Increment TSO occupancy count
	 */
	inline void increment_tso_occupancy(uint32_t port, uint32_t queue,
			uint16_t amount);

	/**
	 * Decrement TSO occupancy count
	 */
	inline void decrement_tso_occupancy(uint32_t port, uint32_t queue,
			uint16_t amount);

private:
	uint32_t m_n_ports;

	uint32_t m_n_queues;

	/** a queue of packets for each port */
	std::vector<struct circular_queue *> m_queues;

	/** the last time the queue was empty, for each port */
	std::vector<uint64_t> m_q_times;

	/** a mask with 1 for non-empty ports, 0 for empty ports */
	uint64_t *m_non_empty_ports;

	/** a mask with 1 for non-empty queues, 0 for empty queues */
	uint64_t *m_non_empty_queues;

	/** logging stats */
	struct queue_bank_stats m_stats;

	/** special occupancy counts used for TSO */
	std::vector<uint32_t> m_tso_occupancies;
};


typedef QueueBank<struct emu_packet> PacketQueueBank;

/** implementation */

template <typename ELEM >
QueueBank<ELEM>::QueueBank(uint32_t n_ports, uint32_t n_queues,
		uint32_t queue_max_size)
	: m_n_ports(n_ports), m_n_queues(n_queues)
{
	uint32_t i;
	uint32_t size_bytes = cq_memsize(queue_max_size);
	struct circular_queue *q;

	if (n_queues > 64)
		throw std::runtime_error("QueueBank: n_queues must be <= 64");
	if (queue_max_size & (queue_max_size - 1))
		throw std::runtime_error("queue_max_size must be a power of 2");

	/* initialize packet queues as empty */
	m_queues.reserve(n_ports * n_queues);

	/* for every queue in the queue bank: */
	for (i = 0; i < (n_ports * n_queues); i++) {
		/* allocate queue */
		q = (struct circular_queue *) fp_malloc("QueueBankQueue", size_bytes);
		if (q == NULL)
			throw std::runtime_error("could not allocate circular queue");
		/* initialize */
		cq_init(q, queue_max_size);
		/* add to queue list */
		m_queues.push_back(q);
	}

	/* initialize all last empty times to zero */
	m_q_times.reserve(n_ports * n_queues);
	for (i = 0; i < (n_ports * n_queues); i++) {
		m_q_times.push_back(0);
	}

	uint32_t mask_n_64 = (n_ports + 63) / 64;
	m_non_empty_ports = (uint64_t *)fp_calloc("non_empty_ports", 1,
			sizeof(uint64_t) * mask_n_64);
	if (m_non_empty_ports == NULL)
		throw std::runtime_error("could not allocate m_non_empty_ports");

	m_non_empty_queues = (uint64_t *)fp_calloc("non_empty_queues", 1,
			sizeof(uint64_t) * n_ports);
	if (m_non_empty_queues == NULL)
		throw std::runtime_error("could not allocate m_non_empty_queues");

	memset(&m_stats, 0, sizeof(m_stats));

	for (i = 0; i < (n_ports * n_queues); i++) {
		m_tso_occupancies.push_back(0);
	}
}

template <typename ELEM >
QueueBank<ELEM>::~QueueBank()
{
	for (uint32_t i = 0; i < (m_n_ports * m_n_queues); i++)
		free(m_queues[i]);

	free(m_non_empty_ports);
	free(m_non_empty_queues);
}

template <typename ELEM >
inline uint32_t QueueBank<ELEM>::flat_index(uint32_t port, uint32_t queue) {
	return (port * m_n_queues) + queue;
}

template <typename ELEM >
inline void QueueBank<ELEM>::enqueue(uint32_t port, uint32_t queue, ELEM *e) {
	uint32_t flat = flat_index(port, queue);

	/* mark port as non-empty */
	asm("bts %1,%0" : "+m" (*m_non_empty_ports) : "r" (port));

	/* mark queue as non_empty */
	asm("bts %1,%0" : "+m" (m_non_empty_queues[port]) : "r" (queue));

	/* enqueue */
	cq_enqueue(m_queues[flat], (void *)e);

	queue_bank_log_enqueue(&m_stats, port);
}

template <typename ELEM >
inline ELEM *QueueBank<ELEM>::dequeue(uint32_t port, uint32_t queue,
		uint64_t cur_time)
{
	uint32_t flat = flat_index(port, queue);
	ELEM *res;

	res = (ELEM *)cq_dequeue(m_queues[flat]);

	uint64_t queue_empty = cq_empty(m_queues[flat]) & 0x1;
	m_non_empty_queues[port] ^= (queue_empty << queue);

	uint64_t port_empty = (m_non_empty_queues[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);

	/* update last queue empty time if the queue has just become empty. note
	 * that if dequeue was called, the queue must have been non-empty before */
	if (queue_empty)
		m_q_times[flat] = cur_time;

	return res;
}

template <typename ELEM >
inline uint64_t *QueueBank<ELEM>::non_empty_port_mask()
{
	return m_non_empty_ports;
}

template <typename ELEM >
inline uint64_t QueueBank<ELEM>::non_empty_queue_mask(uint32_t port)
{
	return m_non_empty_queues[port];
}


template <typename ELEM >
inline int QueueBank<ELEM>::empty(uint32_t port, uint32_t queue)
{
	return cq_empty(m_queues[flat_index(port,queue)]);
}

template <typename ELEM >
inline uint32_t QueueBank<ELEM>:: occupancy(uint32_t port, uint32_t queue)
{
	return cq_occupancy(m_queues[flat_index(port,queue)]);
}

template <typename ELEM >
inline int QueueBank<ELEM>::full(uint32_t port, uint32_t queue)
{
	return cq_full(m_queues[flat_index(port,queue)]);
}

template <typename ELEM >
inline int QueueBank<ELEM>::last_empty_time(uint32_t port, uint32_t queue)
{
	return m_q_times[flat_index(port,queue)];
}

template <typename ELEM >
inline struct queue_bank_stats *QueueBank<ELEM>::get_queue_bank_stats() {
	return &m_stats;
}

template <typename ELEM >
inline uint16_t QueueBank<ELEM>::get_tso_occupancy(uint32_t port,
		uint32_t queue) {
	return m_tso_occupancies[flat_index(port, queue)];
}

template <typename ELEM >
inline void QueueBank<ELEM>::increment_tso_occupancy(uint32_t port,
		uint32_t queue, uint16_t amount) {
	m_tso_occupancies[flat_index(port, queue)] += amount;
}

template <typename ELEM >
inline void QueueBank<ELEM>::decrement_tso_occupancy(uint32_t port,
		uint32_t queue, uint16_t amount) {
	m_tso_occupancies[flat_index(port, queue)] -= amount;
}

#endif /* QUEUE_BANK_H_ */
