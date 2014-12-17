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
	inline ELEM *dequeue(uint32_t port, uint32_t queue);

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

private:
	uint32_t m_n_ports;

	uint32_t m_n_queues;

	std::vector<struct circular_queue *> m_queues;

	/** A mask to extract the lowest @m_log_n_queues bits */
	uint32_t m_queue_mask;

	/** a mask with 1 for non-empty ports, 0 for empty ports */
	uint64_t *m_non_empty_ports;

	/** a mask with 1 for non-empty ports, 0 for empty ports */
	uint64_t *m_non_empty_queues;
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

	m_queues.reserve(n_ports * n_queues);

	/* for every queue in the queue bank: */
	for (i = 0; i < (n_ports * n_queues); i++) {
		/* allocate queue */
		struct circular_queue *q = (struct circular_queue *)malloc(size_bytes);
		if (q == NULL)
			throw std::runtime_error("could not allocate circular queue");
		/* initialize */
		cq_init(q, queue_max_size);
		/* add to queue list */
		m_queues.push_back(q);
	}

	uint32_t mask_n_64 = (n_ports + 63) / 64;
	m_non_empty_ports = (uint64_t *)calloc(1, sizeof(uint64_t) * mask_n_64);
	if (m_non_empty_ports == NULL)
		throw std::runtime_error("could not allocate m_non_empty_ports");

}

template <typename ELEM >
QueueBank<ELEM>::~QueueBank()
{
	for (uint32_t i = 0; i < (m_n_ports << m_n_queues); i++)
		free(m_queues[i]);
}

template <typename ELEM >
inline uint32_t QueueBank<ELEM>::flat_index(uint32_t port, uint32_t queue) {
	return (port << m_n_queues) + queue;
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
}

template <typename ELEM >
inline ELEM *QueueBank<ELEM>::dequeue(uint32_t port, uint32_t queue)
{
	uint32_t flat = flat_index(port, queue);

	return (ELEM *)cq_dequeue(m_queues[flat]);
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

#endif /* QUEUE_BANK_H_ */
