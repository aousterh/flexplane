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
	struct emu_admission_statistics *st = &g_emu_state.stat;
	struct emu_admission_statistics *sv = &emu_saved_admission_statistics;

	printf("\nadmission core (emu with %d nodes)", NUM_NODES);

	printf("\n  allocation fails: %lu packet, %lu admitted",
	       st->emu_packet_alloc_failed, st->emu_admitted_alloc_failed);
	printf("\n  enqueue waits: %lu endpoint, %lu router, ",
	       st->emu_wait_for_endpoint_enqueue,
	       st->emu_wait_for_router_enqueue);
	printf("%lu router internal, %lu admitted",
	       st->emu_wait_for_internal_router_enqueue,
	       st->emu_wait_for_admitted_enqueue);
	printf("\n");

	memcpy(sv, st, sizeof(*sv));
}

void emu_save_admission_stats() {
	memcpy(&emu_saved_admission_statistics, &g_emu_state.stat,
			sizeof(emu_saved_admission_statistics));
}

void emu_save_admission_core_stats(int i) {
	// TODO: copy admission core stats, once they are used
}
