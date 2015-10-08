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
#include "../emulation/emulation_core.h"
#include "../emulation/emu_topology.h"
#include "../emulation/endpoint.h"
#include "../emulation/classifiers/BySourceClassifier.h"
#include "../emulation/queue_managers/drop_tail.h"
#include "../emulation/queue_managers/dctcp.h"
#include "../emulation/queue_managers/pfabric_qm.h"
#include "../emulation/queue_managers/red.h"
#include "../emulation/schedulers/hull_sched.h"
#include "../emulation/packet.h"
#include "../emulation/router.h"
#include "../emulation/util/make_ring.h"
#include "../graph-algo/algo_config.h"

#define TIMESLOTS_PER_ONE_WAY_DELAY 4
#define TIMESLOTS_PER_TIME_SYNC	64

Emulation *g_emulation;
struct emu_topo_config topo_config;

struct admission_log admission_core_logs[RTE_MAX_LCORE];

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000*1000*1000)
#endif

Emulation *emu_get_instance(void)
{
	return g_emulation;
}

struct queue_bank_stats *emu_get_queueing_stats(uint8_t router_index)
{
	return g_emulation->m_queue_bank_stats[router_index];
}

struct port_drop_stats *emu_get_port_stats(uint8_t router_index)
{
	return g_emulation->m_port_drop_stats[router_index];
}

uint16_t emu_get_num_routers()
{
	return num_routers(&topo_config);
}

void emu_admission_init_global(struct rte_ring **q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool)
{
	int i;

	/* init log */
	for (i = 0; i < RTE_MAX_LCORE; i++)
		admission_log_init(&admission_core_logs[i]);

	/* choose topology */
	topo_config.num_racks = EMU_NUM_RACKS;
	topo_config.rack_shift = 5;

	if (IS_STRESS_TEST) {
		RTE_LOG(INFO, ADMISSION,
				"Emulation stress test with %d nodes, %d racks, rack shift of %d\n",
				STRESS_TEST_NUM_NODES, topo_config.num_racks,
				topo_config.rack_shift);
	}

	/* init emu_state */
	enum RouterType rtype;
	void *rtr_args;
	enum EndpointType etype = E_Simple;
#if defined(DCTCP)
	struct dctcp_args dctcp_args;
    dctcp_args.q_capacity = 1024;
    dctcp_args.K_threshold = 65;
	RTE_LOG(INFO, ADMISSION,
			"Using DCTCP routers with q_capacity %d and K threshold %d\n",
			dctcp_args.q_capacity, dctcp_args.K_threshold);

    rtype = R_DCTCP;
    rtr_args = &dctcp_args;
#elif defined(RED)
    struct red_args red_params;
    red_params.q_capacity = 1024;
    red_params.ecn = true;
    red_params.min_th = 150;
    red_params.max_th = 300;
    red_params.max_p = 0.1;
    red_params.wq_shift = 5;
	RTE_LOG(INFO, ADMISSION,
			"Using RED routers with q_capacity %d, ecn %d, min_th %d, max_th %d, max_p %f, wq_shift %d\n",
			red_params.q_capacity, red_params.ecn, red_params.min_th,
			red_params.max_th, red_params.max_p, red_params.wq_shift);

    rtype = R_RED;
    rtr_args = &red_params;
#elif defined(DROP_TAIL) && defined(USE_TSO)
    struct drop_tail_args dt_args;
    dt_args.q_capacity = 1024;
	RTE_LOG(INFO, ADMISSION, "Using DropTail routers with TSO and q_capacity %d\n",
			dt_args.q_capacity);

    rtype = R_DropTailTSO;
    rtr_args = &dt_args;
    etype = E_SimpleTSO;
#elif defined(DROP_TAIL)
    struct drop_tail_args dt_args;
    dt_args.q_capacity = 1024;
	RTE_LOG(INFO, ADMISSION, "Using DropTail routers with q_capacity %d\n",
			dt_args.q_capacity);

    rtype = R_DropTail;
    rtr_args = &dt_args;
#elif defined(PRIO_QUEUEING)
    struct prio_by_src_args prio_args;
    prio_args.q_capacity = 512; /* divide 1024 evenly amongst queues */
    prio_args.n_hi_prio = 24;
    prio_args.n_med_prio = 0;
	RTE_LOG(INFO, ADMISSION,
			"Using Priority Queueing with q_capacity %d, %d hi prio srcs and %d med prio srcs\n",
			prio_args.q_capacity, prio_args.n_hi_prio, prio_args.n_med_prio);
    rtype = R_Prio;
    rtr_args = &prio_args;
#elif defined(PRIO_BY_FLOW_QUEUEING)
    struct drop_tail_args prio_by_flow_args;
    prio_by_flow_args.q_capacity = 256; /* divide 1024 evenly amongst queues */
	RTE_LOG(INFO, ADMISSION,
			"Using Priority Queueing by flow with q_capacity %d\n",
			prio_by_flow_args.q_capacity);
    rtype = R_Prio_by_flow;
    rtr_args = &prio_by_flow_args;
#elif defined(ROUND_ROBIN)
    struct drop_tail_args dt_args;
    dt_args.q_capacity = 32;
	RTE_LOG(INFO, ADMISSION, "Using Round-Robin with q_capacity %d\n",
			dt_args.q_capacity);
    rtype = R_RR;
    rtr_args = &dt_args;
#elif defined(HULL_SCHED)
    struct hull_args hl_args;
    hl_args.q_capacity = 1024;
    hl_args.mark_threshold = 15000;
    hl_args.GAMMA = 0.95;
	RTE_LOG(INFO, ADMISSION,
			"Using HULL SCHED routers with q_capacity %d mark_threshold %d gamma %f\n",
			hl_args.q_capacity, hl_args.mark_threshold, hl_args.GAMMA);

    rtype = R_HULL_sched;
    rtr_args = &hl_args;
#elif defined(PFABRIC)
    struct pfabric_args p_args;
    p_args.q_capacity = PFABRIC_QUEUE_CAPACITY;
    RTE_LOG(INFO, ADMISSION, "Using PFabric routers with q_capacity %d\n",
    		p_args.q_capacity);

    rtype = R_PFabric;
    rtr_args = &p_args;
#else
#error "Unrecognized router type"
#endif

#if defined(USE_TSO)
    RTE_LOG(INFO, ADMISSION, "running with TSO (TCP Segmentation Offload)\n");
#endif

    RTE_LOG(INFO, ADMISSION, "setup info: %d nodes, flow shift %d, comm cores: %d\n",
            NUM_NODES, FLOW_SHIFT, N_COMM_CORES);

	RTE_LOG(INFO, ADMISSION,
			"admitted_traffic_mempool=%p q_admitted_out=%p\n",
			admitted_traffic_mempool, q_admitted_out);

	g_emulation = new Emulation((fp_mempool *) admitted_traffic_mempool,
			(fp_ring **) q_admitted_out, (1 << PACKET_Q_LOG_SIZE), rtype,
			rtr_args, etype, NULL, &topo_config);
}

int exec_emu_admission_core(void *void_cmd_p)
{
	struct admission_core_cmd *cmd = (struct admission_core_cmd *)void_cmd_p;
	uint32_t core_ind = cmd->admission_core_index;
	EmulationCore *core = g_emulation->m_cores[core_ind];
	uint64_t logical_timeslot = cmd->start_timeslot;
	uint64_t time_now, tslot;
    int16_t i;
	/* calculate shift and mul for the rdtsc */
	double tslot_len_seconds = ((double)(1 << TIMESLOT_SHIFT)) / ((double)TIMESLOT_MUL * NSEC_PER_SEC);
    double tslot_len_rdtsc_cycles = tslot_len_seconds * rte_get_timer_hz();
    uint32_t rdtsc_shift = (uint32_t)log(tslot_len_rdtsc_cycles) + 12;
    uint32_t rdtsc_mul = ((double)(1 << rdtsc_shift)) / tslot_len_rdtsc_cycles;

	/* do allocation loop */
	time_now = fp_get_time_ns();
	tslot = (time_now * TIMESLOT_MUL) >> TIMESLOT_SHIFT;

	RTE_LOG(INFO, ADMISSION,
			"core %d admission %d starting allocations, first tslot %lu current %lu\n",
			rte_lcore_id(), core_ind, cmd->start_timeslot, tslot);

	while (1) {
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
