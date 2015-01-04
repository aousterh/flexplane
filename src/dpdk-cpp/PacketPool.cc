/*
 * PacketPool.cc
 *
 *  Created on: Dec 29, 2014
 *      Author: yonch
 */

#include "PacketPool.h"

#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <stdexcept>

dpdk::PacketPool::PacketPool(const std::string &name, unsigned n,
		unsigned elt_size, unsigned cache_size, unsigned private_data_size,
		int socket_id, unsigned flags)
{
	/* Try to allocate the ring */
	m_mempool = rte_mempool_create(name.c_str(), n, elt_size, cache_size,
		private_data_size,
		rte_pktmbuf_pool_init, NULL,
		rte_pktmbuf_init, NULL,
		socket_id, flags);

	if (m_mempool == NULL) {
		std::string msg("Could not allocate packet mbuf mempool name='" + name + "' err=" + rte_strerror(rte_errno));
		throw std::runtime_error(msg);
	}
}

uint32_t dpdk::PacketPool::count() {
	return rte_mempool_count(m_mempool);
}

