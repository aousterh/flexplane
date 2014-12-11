/*
 * admissible_log.c
 *
 *  Created on: September 5, 2014
 *      Author: aousterh
 */

#include "admissible_log.h"
#include "emulation.h"
#include "../arbiter/emu_admission_core.h"
#include "../protocol/topology.h"

#include <stdio.h>

struct emu_admission_statistics emu_saved_admission_statistics;

void print_global_admission_log_emulation() {
	uint64_t dropped_in_algo;
	struct emu_admission_statistics *st = &g_emu_state.stat;
	struct emu_admission_statistics *sv = &emu_saved_admission_statistics;

	printf("\nadmission core (emu with %d nodes)", NUM_NODES);

#define D(X) (st->X - sv->X)
	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			D(wait_for_admitted_enqueue), D(admitted_alloc_failed));
	printf("\n  demands: %lu admitted, %lu dropped",
			D(admitted_packet), D(dropped_demand));
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			D(endpoint_sent_packet), D(router_sent_packet));
	printf("\n  algo dropped %lu from endpoints, %lu from routers",
			D(endpoint_dropped_packet), D(router_dropped_packet));
	printf("\n");
#undef D

	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			st->wait_for_admitted_enqueue, st->admitted_alloc_failed);
	printf("\n  demands: %lu admitted, %lu dropped",
			st->admitted_packet, st->dropped_demand);
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			st->endpoint_sent_packet, st->router_sent_packet);
	printf("\n  algo dropped %lu from endpoints, %lu from routers",
			st->endpoint_dropped_packet, st->router_dropped_packet);

	printf("\n errors:");
	if (st->admitted_struct_overflow)
		printf("\n  %lu admitted struct overflow (too many drops?)",
				st->admitted_struct_overflow);

	printf("\n warnings:");
	if (st->packet_alloc_failed)
		printf("\n  %lu packet allocs failed (increase packet mempool size?)",
				st->packet_alloc_failed);
	if (st->endpoint_enqueue_backlog_failed)
		printf("\n  %lu endpoint enqueue backlog failed",
				st->endpoint_enqueue_backlog_failed);
	if (st->send_packet_failed)
		printf("\n  %lu send packet failed", st->send_packet_failed);
	printf("\n");
}

void emu_save_admission_stats() {
	memcpy(&emu_saved_admission_statistics, &g_emu_state.stat,
			sizeof(emu_saved_admission_statistics));
}

void emu_save_admission_core_stats(int i) {
	// TODO: copy admission core stats, once they are used
}
