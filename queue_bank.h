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
	 * @param n_queues: the number of queues per port
	 * @param queue_max_size: the maximum number of packets each queue can hold
	 *
	 * @important: queue_max_size must be a power of two
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
	inline void enqueue(uint32_t queue_index, ELEM *e);

	/**
	 * Dequeues an element
	 * @param queue_index: the flat index of the queue to dequeue
	 * @assumes the queue is non-empty
	 */
	inline ELEM *dequeue(uint32_t queue_index);

	/**
	 * @returns 1 if queue is empty, 0 otherwise
	 */
	inline int empty(uint32_t queue_index);

	/**
	 * @returns the number of elements in queue
	 * @param queue_index: the flat index of the queue
	 */
	inline uint32_t occupancy(uint32_t queue_index);

	/**
	 * @returns 1 if queue is empty, 0 otherwise
	 */
	inline int full(uint32_t queue_index);

private:
	uint32_t m_n_ports;

	uint32_t m_n_queues;

	std::vector<struct circular_queue *> m_queues;
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
	for (i = 0; i < n_ports * n_queues; i++) {
		/* allocate queue */
		struct circular_queue *q = (struct circular_queue *)malloc(size_bytes);
		if (q == NULL)
			throw std::runtime_error("could not allocate circular queue");
		/* initialize */
		cq_init(q, queue_max_size);
		/* add to queue list */
		m_queues.push_back(q);
	}
}

template <typename ELEM >
QueueBank<ELEM>::~QueueBank()
{
	for (uint32_t i = 0; i < m_n_ports * m_n_queues; i++)
		free(m_queues[i]);
}

template <typename ELEM >
inline uint32_t QueueBank<ELEM>::flat_index(uint32_t port, uint32_t queue) {
	return (port * m_n_queues) + queue;
}

template <typename ELEM >
inline void QueueBank<ELEM>::enqueue(uint32_t queue_index, ELEM *e) {
	cq_enqueue(m_queues[queue_index], (void *)e);
}

template <typename ELEM >
inline ELEM *QueueBank<ELEM>::dequeue(uint32_t queue_index)
{
	return (ELEM *)cq_dequeue(m_queues[queue_index]);
}

template <typename ELEM >
inline int QueueBank<ELEM>::empty(uint32_t queue_index)
{
	return cq_empty(m_queues[queue_index]);
}

template <typename ELEM >
inline uint32_t QueueBank<ELEM>:: occupancy(uint32_t queue_index)
{
	return cq_occupancy(m_queues[queue_index]);
}

template <typename ELEM >
inline int QueueBank<ELEM>::full(uint32_t queue_index)
{
	return cq_full(m_queues[queue_index]);
}

#endif /* QUEUE_BANK_H_ */
