/*
 * MemPool.cc
 *
 *  Created on: Dec 29, 2014
 *      Author: yonch
 */

#include "MemPool.h"

#include <rte_mempool.h>
#include <rte_errno.h>

dpdk::MemPool::MemPool(const std::string &name, unsigned n,
		unsigned elt_size, unsigned cache_size, int socket_id, unsigned flags)
{
	/* Try to allocate the ring */
	m_mempool = rte_mempool_create(name.c_str(), n, elt_size, cache_size,
		0, NULL, NULL, NULL, NULL, socket_id, flags);

	if (m_mempool == NULL) {
		std::string msg("Could not allocate packet mbuf mempool name='" + name + "' err=" + rte_strerror(rte_errno));
		throw std::runtime_error(msg);
	}
}

uint32_t dpdk::MemPool::count() {
	return rte_mempool_count(m_mempool);
}

