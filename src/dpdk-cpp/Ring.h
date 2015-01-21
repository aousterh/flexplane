/*
 * Ring.h
 *
 *  Created on: Dec 20, 2014
 *      Author: yonch
 */

#ifndef RING_H_
#define RING_H_

#define typeof(x) __typeof__(x)
#include <rte_ring.h>
#include <rte_errno.h>
#include <stdexcept>
#include <string>

class Ring {
public:
	Ring(const std::string &name, unsigned count, int socket_id,
			unsigned flags);

	/**
	 * @returns the struct rte_ring *
	 */
	inline struct rte_ring *get();

	inline int enqueue(void *elem);
	inline int enqueue_bulk(void **elems, unsigned n);
	inline int dequeue(void **obj_p);
	inline int dequeue_burst(void **elems, unsigned n);

	inline int empty();

private:
	struct rte_ring *m_ring;
};

/** implementation */

inline Ring::Ring(const std::string& name, unsigned count, int socket_id,
		unsigned flags)
{
	/* Try to allocate the ring */
	m_ring = rte_ring_create(name.c_str(), count, socket_id, flags);

	if (m_ring == NULL) {
		std::string msg("Could not allocate ring name='" + name + "' err=" + rte_strerror(rte_errno));
		throw std::runtime_error(msg);
	}
}

inline struct rte_ring* __attribute__((always_inline))
Ring::get()
{
	return m_ring;
}

inline int __attribute__((always_inline))
Ring::enqueue(void *elem)
{
	return rte_ring_enqueue(m_ring, (void *)elem);
}

inline int __attribute__((always_inline))
Ring::enqueue_bulk(void **elems, unsigned n)
{
	return rte_ring_enqueue_bulk(m_ring, (void **)elems, n);
}

inline int __attribute__((always_inline))
Ring::dequeue(void **obj_p)
{
	return rte_ring_dequeue(m_ring, (void **)obj_p);
}

inline int __attribute__((always_inline))
Ring::dequeue_burst(void **elems, unsigned n)
{
	return rte_ring_dequeue_burst(m_ring, (void **)elems, n);
}

inline int __attribute__((always_inline))
Ring::empty() {
	return rte_ring_empty(m_ring);
}

#endif /* RING_H_ */
