/*
 * emu_admission_core.c
 *
 *  Created on: July 12, 2014
 *      Author: aousterh
 */

#include "emu_admission_core.h"

#include <rte_errno.h>
#include <rte_string_fns.h>

#include "admission_core_common.h"
#include "admission_log.h"
#include "../emulation/admitted.h"
#include "../emulation/emulation.h"
#include "../graph-algo/algo_config.h"

#define EMU_N_PACKET_QUEUES   (EMU_NUM_ENDPOINTS + EMU_NUM_ROUTERS + \
                               EMU_NUM_ROUTERS * EMU_ROUTER_MAX_ENDPOINT_PORTS)
#define ROUTER_OUTPUT_PORT_CAPACITY 512

struct emu_state g_emu_state;

struct rte_mempool* admitted_traffic_pool[NB_SOCKETS];
struct admission_log admission_core_logs[RTE_MAX_LCORE];
struct rte_ring *packet_queues[EMU_N_PACKET_QUEUES];

void emu_admission_init_global(struct rte_ring *q_admitted_out)
{
	int i;
	char s[64];
	struct rte_mempool *packet_mempool;

	/* allocate packet_mempool */
	uint32_t pool_index = 0;
	uint32_t socketid = 0;
	rte_snprintf(s, sizeof(s), "packet_pool_%d", pool_index);
	packet_mempool =
                rte_mempool_create(s,
                                   PACKET_MEMPOOL_SIZE, /* num elements */
                                   sizeof(struct emu_packet), /* element size */
                                   PACKET_MEMPOOL_CACHE_SIZE, /* cache size */
                                   0, NULL, NULL, NULL, NULL, /* custom init, disabled */
                                   socketid, 0);
	if (packet_mempool == NULL)
		rte_exit(EXIT_FAILURE,
                         "Cannot init packet mempool on socket %d: %s\n",
                         socketid, rte_strerror(rte_errno));
	else
		RTE_LOG(INFO, ADMISSION,
                        "Allocated packet mempool on socket %d - %lu bufs\n",
			socketid, (uint64_t) PACKET_MEMPOOL_SIZE);

	/* allocate admitted_traffic_mempool */
	pool_index = 0;
	if (admitted_traffic_pool[pool_index] == NULL) {
		rte_snprintf(s, sizeof(s), "admitted_traffic_pool_%d",
                             pool_index);
		admitted_traffic_pool[pool_index] =
                        rte_mempool_create(s,
                                           ADMITTED_TRAFFIC_MEMPOOL_SIZE, /* num elements */
                                           sizeof(struct emu_admitted_traffic), /* element size */
                                           ADMITTED_TRAFFIC_CACHE_SIZE, /* cache size */
                                           0, NULL, NULL, NULL, NULL, /* custom init, disabled */
                                           socketid, 0);
		if (admitted_traffic_pool[pool_index] == NULL)
			rte_exit(EXIT_FAILURE,
                                 "Cannot init admitted traffic pool on socket %d: %s\n",
                                 socketid, rte_strerror(rte_errno));
		else
			RTE_LOG(INFO, ADMISSION,
                                "Allocated admitted traffic pool on socket %d - %lu bufs\n",
                                socketid, (uint64_t)ADMITTED_TRAFFIC_MEMPOOL_SIZE);
	}

	/* init log */
	for (i = 0; i < RTE_MAX_LCORE; i++)
		admission_log_init(&admission_core_logs[i]);

	/* init packet_queues */
	for (i = 0; i < EMU_N_PACKET_QUEUES; i++) {
		rte_snprintf(s, sizeof(s), "packet_q_%d", i);
		packet_queues[i] = rte_ring_create(s, PACKET_Q_SIZE, 0,
                                                   RING_F_SP_ENQ | RING_F_SC_DEQ);
		if (packet_queues[i] == NULL)
			rte_exit(EXIT_FAILURE,
                                 "Cannot init packet_queues[%d]: %s\n", i,
                                 rte_strerror(rte_errno));
	}
        RTE_LOG(INFO, ADMISSION, "Initialized %d packet queues of size %d\n",
                EMU_N_PACKET_QUEUES, PACKET_Q_SIZE);

	/* init emu_state */
	emu_init_state(&g_emu_state, admitted_traffic_pool[0], q_admitted_out,
                       packet_mempool, packet_queues,
                       ROUTER_OUTPUT_PORT_CAPACITY);
}

int exec_emu_admission_core(void *void_cmd_p)
{
	struct admission_core_cmd *cmd = (struct admission_core_cmd *)void_cmd_p;
	uint32_t core_ind = cmd->admission_core_index;
	uint64_t logical_timeslot = cmd->start_timeslot;
	uint64_t start_time_first_timeslot;

	ADMISSION_DEBUG("core %d admission %d starting allocations\n",
			rte_lcore_id(), core_ind);

	/* do allocation loop */
        while (1) {
		admission_log_allocation_begin(logical_timeslot,
				start_time_first_timeslot);

		/* perform allocation */
                /* use a single core for now */
                emu_timeslot(&g_emu_state);

		admission_log_allocation_end(logical_timeslot);

		logical_timeslot += 1;
	}

	return 0;
}
