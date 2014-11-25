
#ifndef CONTROLLER_DPDK_PLATFORM_H_
#define CONTROLLER_DPDK_PLATFORM_H_

#include <time.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include "main.h"
#include "dpdk-time.h"

extern struct rte_mempool* pktdesc_pool[NB_SOCKETS];

#define RTE_LOGTYPE_ARBITER RTE_LOGTYPE_USER1

#ifdef CONFIG_IP_FASTPASS_DEBUG
#define ARBITER_DEBUG(a...) RTE_LOG(DEBUG, ARBITER, ##a)
#else
#define ARBITER_DEBUG(a...)
#endif

#define FASTPASS_PR_DEBUG(enable, fmt, a...)	do { if (enable)	\
							ARBITER_DEBUG("%s: " fmt, __func__, ##a); \
						} while(0)

static inline
struct fpproto_pktdesc *fpproto_pktdesc_alloc(void)
{
	struct fpproto_pktdesc *pd;
	int socketid = rte_lcore_to_socket_id(rte_lcore_id());

	if (unlikely(rte_mempool_get(pktdesc_pool[socketid], (void**)&pd) != 0))
			return NULL;

	return pd;
}

static inline
void fpproto_pktdesc_free(struct fpproto_pktdesc *pd)
{
	int socketid = rte_lcore_to_socket_id(rte_lcore_id());
	rte_mempool_put(pktdesc_pool[socketid], pd);
}

#endif /* CONTROLLER_DPDK_PLATFORM_H_ */
