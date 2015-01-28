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
	struct emu_admission_core_statistics *st = g_emu_state.core_stats[core_index];
	struct emu_admission_core_statistics *sv = &emu_saved_admission_core_statistics[core_index];

	printf("\nadmission core %d", core_index);
#define D(X) (st->X - sv->X)
	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			D(wait_for_admitted_enqueue), D(admitted_alloc_failed));
	printf("\n  demands: %lu admitted", D(admitted_packet));
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			D(endpoint_sent_packet), D(router_sent_packet));
	printf("\n  algo dropped %lu from endpoints, %lu from routers",
			D(endpoint_dropped_packet), D(router_dropped_packet));
	printf("\n  algo marked %lu from routers", D(router_marked_packet));
	printf("\n");
#undef D

	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			st->wait_for_admitted_enqueue, st->admitted_alloc_failed);
	printf("\n  demands: %lu admitted", st->admitted_packet);
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			st->endpoint_sent_packet, st->router_sent_packet);
	printf("\n  algo dropped %lu from endpoints, %lu from routers",
			st->endpoint_dropped_packet, st->router_dropped_packet);
	printf("\n  algo marked %lu from routers", st->router_marked_packet);

	printf("\n errors:");
	if (st->admitted_struct_overflow)
		printf("\n  %lu admitted struct overflow (too many drops?)",
				st->admitted_struct_overflow);

	printf("\n warnings:");
	if (st->send_packet_failed)
		printf("\n  %lu send packet failed", st->send_packet_failed);
	printf("\n");
}

void print_global_admission_log_emulation() {
	uint64_t dropped_in_algo;
	struct emu_admission_statistics *st = &g_emu_state.stat;
	struct emu_admission_statistics *sv = &emu_saved_admission_statistics;

	printf("\nemulation with %d nodes", NUM_NODES);

#if defined(DCTCP)
	printf("\nrouter type DCTCP");
#elif defined(RED)
	printf("\nrouter type RED");
#elif defined(DROP_TAIL)
	printf("\nrouter type drop tail");
#endif

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
	memcpy(&emu_saved_admission_statistics, &g_emu_state.stat,
			sizeof(emu_saved_admission_statistics));
}

void emu_save_admission_core_stats(int core_index) {
	uint16_t i;

	memcpy(&emu_saved_admission_core_statistics[core_index],
			g_emu_state.core_stats[core_index],
			sizeof(struct emu_admission_core_statistics));
}
