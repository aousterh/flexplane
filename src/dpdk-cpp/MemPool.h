/*
 * MemPool.h
 *
 *  Created on: Dec 29, 2014
 *      Author: yonch
 */

#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <string>
#include <stdint.h>
#include <stdexcept>

struct rte_mempool;

namespace dpdk {

class MemPool {
public:
	MemPool(const std::string &name, unsigned n, unsigned elt_size,
			unsigned cache_size, int socket_id, unsigned flags);

	uint32_t count(); /* note, expensive operation, only use to debug */

	inline struct rte_mempool* get();

	template <class T>
	inline T *alloc();

	template <class T>
	inline void free(T *m);

private:
	struct rte_mempool *m_mempool;
};

} /* namespace dpdk */

/* implementation */

inline struct rte_mempool* __attribute__((always_inline))
dpdk::MemPool::get()
{
	return m_mempool;
}

template <typename T>
inline T* __attribute__((always_inline))
dpdk::MemPool::alloc()
{
	T *objp;
	int ret = rte_mempool_get(m_mempool, &objp);
	if (ret != 0)
		throw std::runtime_error("couldn't allocate mempool entry");
	return objp;
}

template <typename T>
inline void __attribute__((always_inline))
dpdk::MemPool::free(T* m)
{
	rte_mempool_put(m_mempool, m);
}

#endif /* MEMPOOL_H_ */
