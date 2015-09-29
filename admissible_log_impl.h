/*
 * admissible_log_impl.h
 *
 *  Created on: September 5, 2014
 *      Author: aousterh
 */

#include "admissible_log.h"
#include "emulation.h"
#include "../arbiter/emu_admission_core.h"
#include "../protocol/topology.h"

#include <stdio.h>

struct emu_admission_core_statistics emu_saved_admission_core_statistics[ALGO_N_CORES];
struct emu_admission_statistics emu_saved_admission_statistics;

void print_core_admission_log_emulation(uint16_t core_index) {
	uint64_t dropped_in_algo;
	Emulation *emulation = emu_get_instance();
	struct emu_admission_core_statistics *st =
			emulation->m_core_stats[core_index];
	struct emu_admission_core_statistics *sv =
			&emu_saved_admission_core_statistics[core_index];

	printf("\nadmission core %d", core_index);
#define D(X) (st->X - sv->X)
/*	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			D(wait_for_admitted_enqueue), D(admitted_alloc_failed));
	printf("\n  packets: %lu admitted, %lu dropped, %lu marked",
			D(admitted_packet), D(dropped_packet), D(marked_packet));*/
	printf("\n  endpoint driver pushed %lu, pulled %lu, new %lu",
			D(endpoint_driver_pushed), D(endpoint_driver_pulled),
			D(endpoint_driver_processed_new));
	printf("\n  router driver pushed %lu, pulled %lu", D(router_driver_pushed),
			D(router_driver_pulled));
	printf("\n");

/*	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			st->wait_for_admitted_enqueue, st->admitted_alloc_failed);
	printf("\n  packets: %lu admitted, %lu dropped, %lu marked",
			st->admitted_packet, st->dropped_packet, st->marked_packet);*/

	printf("\n  endpoint driver pushed %lu, pulled %lu, new %lu, push begin %lu, pull begin %lu, new begin %lu",
			st->endpoint_driver_pushed, st->endpoint_driver_pulled,
			st->endpoint_driver_processed_new, st->endpoint_driver_push_begin, st->endpoint_driver_pull_begin, st->endpoint_driver_new_begin);
	printf("\n  router driver pushed %lu, pulled %lu, steps begun %lu, steps ended %lu",
			st->router_driver_pushed, st->router_driver_pulled, st->router_driver_step_begin, st->router_driver_step_end);

	printf("\n warnings:");
	if (st->send_packet_failed)
		printf("\n  %lu send packet failed", st->send_packet_failed);
	if (D(wait_for_admitted_enqueue))
		printf("\n  %lu admitted waits", D(wait_for_admitted_enqueue));
	if (D(admitted_alloc_failed))
		printf("\n  %lu admitted alloc fails", D(admitted_alloc_failed));
	printf("\n");
#undef D
}

void print_global_admission_log_emulation() {
	uint64_t dropped_in_algo;
	Emulation *emulation = emu_get_instance();
	struct emu_admission_statistics *st = &emulation->m_stat;
	struct emu_admission_statistics *sv = &emu_saved_admission_statistics;
	uint16_t core_index;
	struct emu_admission_core_statistics *core_st, *core_sv;
	uint64_t total_router_pulled = 0;
	double gbps;

	printf("\nemulation with %d nodes, ", NUM_NODES);

#if defined(DCTCP)
	printf("router type DCTCP");
#elif defined(RED)
	printf("router type RED");
#elif defined(DROP_TAIL)
	printf("router type drop tail");
#elif defined(PRIO_QUEUEING)
	printf("router type priority");
#elif defined(PRIO_BY_FLOW_QUEUEING)
	printf("router type priority by flow");
#elif defined(ROUND_ROBIN)
	printf("router type round robin");
#elif defined(HULL)
	printf("router type HULL");
#elif defined(HULL_SCHED)
	printf("router type HULL as scheduler");
#elif defined(PFABRIC)
	printf("router type pFabric");
#endif

#if defined(USE_TSO)
	printf(", using TSO");
#endif

	/* compute and print total throughput across all routers */
	for (core_index = 0; core_index < ALGO_N_CORES; core_index++) {
		core_st = emulation->m_core_stats[core_index];
		core_sv = &emu_saved_admission_core_statistics[core_index];
		total_router_pulled += core_st->router_driver_pulled -
				core_sv->router_driver_pulled;
	}
	/* assuming logging gap is 0.1 seconds */
	gbps = total_router_pulled * 1500 * 8 / (0.1 * 1000 * 1000 * 1000);
	printf("\ntotal router throughput: %f", gbps);

	printf("\n warnings:");
	if (st->packet_alloc_failed)
		printf("\n  %lu packet allocs failed (increase packet mempool size?)",
				st->packet_alloc_failed);
	if (st->enqueue_backlog_failed)
		printf("\n  %lu enqueue backlog failed", st->enqueue_backlog_failed);
	if (st->enqueue_reset_failed)
		printf("\n  %lu enqueue reset failed", st->enqueue_reset_failed);
	printf("\n");
}

void emu_save_admission_stats() {
	Emulation *emulation = emu_get_instance();
	memcpy(&emu_saved_admission_statistics, &emulation->m_stat,
			sizeof(emu_saved_admission_statistics));
}

void emu_save_admission_core_stats(int core_index) {
	uint16_t i;

	Emulation *emulation = emu_get_instance();
	memcpy(&emu_saved_admission_core_statistics[core_index],
			emulation->m_core_stats[core_index],
			sizeof(struct emu_admission_core_statistics));
}
