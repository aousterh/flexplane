/*
 * emu_admission_core.c
 *
 *  Created on: July 12, 2014
 *      Author: aousterh
 */

#include "emu_admission_core.h"

#include <rte_errno.h>
#include <rte_string_fns.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>

#include "admission_core_common.h"
#include "admission_log.h"
#include "../emulation/admitted.h"
#include "../emulation/emulation.h"
#include "../emulation/endpoint.h"
#include "../emulation/queue_managers/drop_tail.h"
#include "../emulation/queue_managers/dctcp.h"
#include "../emulation/queue_managers/red.h"
#include "../emulation/queue_managers/hull.h"
#include "../emulation/packet.h"
#include "../emulation/router.h"
#include "../emulation/util/make_ring.h"
#include "../graph-algo/algo_config.h"

#define TIMESLOTS_PER_ONE_WAY_DELAY 4
#define TIMESLOTS_PER_TIME_SYNC	64

struct emu_state g_emu_state;

struct admission_log admission_core_logs[RTE_MAX_LCORE];
struct rte_ring *packet_queues[EMU_NUM_PACKET_QS];

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000*1000*1000)
#endif

void emu_admission_init_global(struct rte_ring *q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool)
{
	int i;
	char s[64];
	struct rte_mempool *packet_mempool;
	uint32_t packet_size;

	/* setup for specific emulation algorithm */
	packet_size = EMU_ALIGN(sizeof(struct emu_packet)) + 0;

	/* allocate packet_mempool */
	uint32_t pool_index = 0;
	uint32_t socketid = 0;
	snprintf(s, sizeof(s), "packet_pool_%d", pool_index);
	packet_mempool =
			rte_mempool_create(s, PACKET_MEMPOOL_SIZE, /* num elements */
					packet_size, /* element size */
					PACKET_MEMPOOL_CACHE_SIZE, /* cache size */
					0, NULL, NULL, NULL, NULL, /* custom init, disabled */
					socketid, 0);
	if (packet_mempool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init packet mempool on socket %d: %s\n",
				socketid, rte_strerror(rte_errno));
	else
		RTE_LOG(INFO, ADMISSION,
				"Allocated packet mempool on socket %d - %lu bufs\n", socketid,
				(uint64_t) PACKET_MEMPOOL_SIZE);

	/* init log */
	for (i = 0; i < RTE_MAX_LCORE; i++)
		admission_log_init(&admission_core_logs[i]);

	/* init packet_queues */
	for (i = 0; i < EMU_NUM_PACKET_QS; i++) {
		snprintf(s, sizeof(s), "packet_q_%d", i);
		packet_queues[i] = make_ring(s, PACKET_Q_SIZE, 0, RING_F_SC_DEQ);
	}
	RTE_LOG(INFO, ADMISSION, "Initialized %d packet queues of size %d\n",
			EMU_NUM_PACKET_QS, PACKET_Q_SIZE);

	/* init emu_state */
	enum RouterType rtype;
	void *rtr_args;
#if defined(DCTCP)
	struct dctcp_args dctcp_args;
    dctcp_args.q_capacity = 512;
    dctcp_args.K_threshold = 64;
	RTE_LOG(INFO, ADMISSION,
			"Using DCTCP routers with q_capacity %d and K threshold %d\n",
			dctcp_args.q_capacity, dctcp_args.K_threshold);

    rtype = R_DCTCP;
    rtr_args = &dctcp_args;
#elif defined(RED)
    struct red_args red_params;
    red_params.q_capacity = 512;
    red_params.ecn = true;
    red_params.min_th = 50;
    red_params.max_th = 250;
    red_params.max_p = 0.1;
    red_params.wq_shift = 9;
	RTE_LOG(INFO, ADMISSION,
			"Using RED routers with q_capacity %d, ecn %d, min_th %d, max_th %d, max_p %f, wq_shift %d\n",
			red_params.q_capacity, red_params.ecn, red_params.min_th,
			red_params.max_th, red_params.max_p, red_params.wq_shift);

    rtype = R_RED;
    rtr_args = &red_params;
#elif defined(DROP_TAIL)
    struct drop_tail_args dt_args;
    dt_args.q_capacity = 512;
	RTE_LOG(INFO, ADMISSION, "Using DropTail routers with q_capacity %d\n",
			dt_args.q_capacity);

    rtype = R_DropTail;
    rtr_args = &dt_args;
#elif defined(PRIO_QUEUEING)
	RTE_LOG(INFO, ADMISSION, "Using Priority Queueing with default parameters\n");
    rtype = R_Prio;
    rtr_args = NULL;
#elif defined(ROUND_ROBIN)
	RTE_LOG(INFO, ADMISSION, "Using Round-Robin with default parameters\n");
    rtype = R_RR;
    rtr_args = NULL;
#elif defined(HULL)
    struct hull_args hl_args;
    hl_args.q_capacity = 512;
	RTE_LOG(INFO, ADMISSION, "Using HULL routers with q_capacity %d mark_threshold %d gamma %f\n",
			hl_args.q_capacity, hl_args.mark_threshold, hl_args.GAMMA);

    rtype = R_HULL;
    rtr_args = &hl_args;
#else
#error "Unrecognized router type"
#endif

	RTE_LOG(INFO, ADMISSION,
			"admitted_traffic_mempool=%p q_admitted_out=%p packet_mempool=%p\n",
			admitted_traffic_mempool, q_admitted_out, packet_mempool);

	emu_init_state(&g_emu_state, (fp_mempool *) admitted_traffic_mempool,
			(fp_ring *) q_admitted_out, (fp_mempool *) packet_mempool,
            (fp_ring **) packet_queues, rtype, rtr_args, E_Simple, NULL);


}

int exec_emu_admission_core(void *void_cmd_p)
{
	struct admission_core_cmd *cmd = (struct admission_core_cmd *)void_cmd_p;
	uint32_t core_ind = cmd->admission_core_index;
	EmulationCore *core = g_emu_state.cores[core_ind];
	int ret;
	uint64_t logical_timeslot = cmd->start_timeslot;
	uint64_t start_time_first_timeslot, time_now, tslot;
	int64_t timeslots_behind;
    int16_t i;
	/* calculate shift and mul for the rdtsc */
	double tslot_len_seconds = ((double)(1 << TIMESLOT_SHIFT)) / ((double)TIMESLOT_MUL * NSEC_PER_SEC);
    double tslot_len_rdtsc_cycles = tslot_len_seconds * rte_get_timer_hz();
    uint32_t rdtsc_shift = (uint32_t)log(tslot_len_rdtsc_cycles) + 12;
    uint32_t rdtsc_mul = ((double)(1 << rdtsc_shift)) / tslot_len_rdtsc_cycles;

	/* set thread priority to max */
/*	pthread_t this_thread = pthread_self();
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
	if (ret != 0) {
	  ADMISSION_DEBUG("core %d admission %d failed to set thread realtime priority\n",
			  rte_lcore_id(), core_ind);
	} else {
	  ADMISSION_DEBUG("core %d admission %d successfully set thread realtime priority\n",
			  rte_lcore_id(), core_ind);
                          }*/

	/* do allocation loop */
	time_now = fp_get_time_ns();
	tslot = (time_now * TIMESLOT_MUL) >> TIMESLOT_SHIFT;

	RTE_LOG(INFO, ADMISSION,
			"core %d admission %d starting allocations, first tslot %lu current %lu\n",
			rte_lcore_id(), core_ind, cmd->start_timeslot, tslot);

	while (1) {
		/* check if we're behind */
/*		timeslots_behind = tslot - logical_timeslot +
				TIMESLOTS_PER_ONE_WAY_DELAY;
		if (timeslots_behind > TSLOTS_BEHIND_TOLERANCE) {*/
			/* skip timeslots to catch up */
		/*	logical_timeslot += timeslots_behind;

			admission_log_core_skipped_tslots(timeslots_behind);
		}*/

		/* re-calibrate clock */
		uint64_t real_time = fp_get_time_ns();
		uint64_t rdtsc_time = rte_get_timer_cycles();
		uint64_t real_tslot = (real_time * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
		uint64_t rdtsc_tslot = (rdtsc_time * rdtsc_mul) >> rdtsc_shift;
		tslot = real_tslot;

		for (i = 0; i < TIMESLOTS_PER_TIME_SYNC; i++) {

			/* pace emulation so that timeslots arrive at endpoints just in time */
			while (tslot < logical_timeslot - TIMESLOTS_PER_ONE_WAY_DELAY) {
				admission_log_core_ahead();
				rdtsc_time = rte_get_timer_cycles();
				tslot = ((rdtsc_time * rdtsc_mul) >> rdtsc_shift) +
						(real_tslot - rdtsc_tslot);
			}

			admission_log_allocation_begin(logical_timeslot);

			/* perform allocation on this core */
			core->step();

			admission_log_allocation_end(logical_timeslot);

			logical_timeslot += 1;
		}
	}

	return 0;
}
