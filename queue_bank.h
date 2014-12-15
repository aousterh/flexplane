/*
 * queue_bank.h
 *
 *  Created on: Dec 15, 2014
 *      Author: yonch
 */

#ifndef QUEUE_BANK_H_
#define QUEUE_BANK_H_

#include <vector>
#include <cstdint>
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

private:
	uint32_t m_n_ports;

	uint32_t m_n_queues;

	std::vector<struct circular_queue *> m_queues;
};


/** implementation */


template <typename ELEM >
QueueBank<ELEM>::QueueBank(uint32_t n_ports, uint32_t n_queues,
		uint32_t queue_max_size)
	: m_n_ports(n_ports), m_n_queues(n_queues)
{
	uint32_t i;
	uint32_t size_bytes = cq_memsize(queue_max_size);

	m_queues.reserve(n_ports * n_queues);
	for (i = 0; i < n_ports * n_queues; i++) {
		struct circular_queue *q = malloc(size_bytes);
		if (q == NULL)
			throw Run
	}
}
template <typename ELEM >
QueueBank<ELEM>::~QueueBank();
template <typename ELEM >
inline uint32_t QueueBank<ELEM>::flat_index(uint32_t port, uint32_t queue);
template <typename ELEM >
inline void QueueBank<ELEM>::enqueue(uint32_t queue_index, ELEM *e);
template <typename ELEM >
inline ELEM *QueueBank<ELEM>::dequeue(uint32_t queue_index);
template <typename ELEM >
inline int QueueBank<ELEM>::empty(uint32_t queue_index);


#endif /* QUEUE_BANK_H_ */
