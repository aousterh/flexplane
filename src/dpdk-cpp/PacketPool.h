/*
 * PacketPool.h
 *
 *  Created on: Dec 29, 2014
 *      Author: yonch
 */

#ifndef PACKETPOOL_H_
#define PACKETPOOL_H_

#include <string>
#include <stdint.h>
#include <rte_mbuf.h>
#include <stdexcept>

struct rte_mempool;

namespace dpdk {

class PacketPool {
public:
	PacketPool(const std::string &name, unsigned n, unsigned elt_size,
			   unsigned cache_size, int socket_id, unsigned flags);

	uint32_t count(); /* note, expensive operation, only use to debug */

	inline struct rte_mempool* get();

	inline struct rte_mbuf *alloc();
	static inline void free(struct rte_mbuf *m);

private:
	struct rte_mempool *m_mempool;
};

} /* namespace dpdk */

/* implementation */

inline struct rte_mempool* __attribute__((always_inline))
dpdk::PacketPool::get()
{
	return m_mempool;
}

inline struct rte_mbuf* __attribute__((always_inline))
dpdk::PacketPool::alloc()
{
	struct rte_mbuf *ret = rte_pktmbuf_alloc(m_mempool);
	if (ret == NULL)
		throw std::runtime_error("couldn't allocate pktmbuf");
	return ret;
}

inline void __attribute__((always_inline))
dpdk::PacketPool::free(struct rte_mbuf* m)
{
	rte_pktmbuf_free(m);
}


#endif /* PACKETPOOL_H_ */
