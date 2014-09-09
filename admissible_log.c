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
	printf("\n  drops: %lu packet alloc fail, %lu ep enqueue, ",
	       D(packet_alloc_failed), D(endpoint_enqueue_backlog_failed));
	dropped_in_algo = D(dropped_demand) - D(packet_alloc_failed) -
		D(endpoint_enqueue_backlog_failed) - D(send_packet_failed) -
		D(endpoint_enqueue_received_failed);
	printf("%lu send, %lu ep receive, %lu in algo", D(send_packet_failed),
	       D(endpoint_enqueue_received_failed), dropped_in_algo);
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			D(endpoint_sent_packet), D(router_sent_packet));
	printf("\n");
#undef D

	printf("\n  admitted waits: %lu, admitted alloc fails: %lu",
			st->wait_for_admitted_enqueue, st->admitted_alloc_failed);
	printf("\n  demands: %lu admitted, %lu dropped",
			st->admitted_packet, st->dropped_demand);
	printf("\n  drops: %lu packet alloc fail, %lu ep enqueue, ",
			st->packet_alloc_failed, st->endpoint_enqueue_backlog_failed);
	dropped_in_algo = st->dropped_demand - st->packet_alloc_failed -
			st->endpoint_enqueue_backlog_failed - st->send_packet_failed -
			st->endpoint_enqueue_received_failed;
	printf("%lu send, %lu ep receive, %lu in algo", st->send_packet_failed,
			st->endpoint_enqueue_received_failed, dropped_in_algo);
	printf("\n  algo sent %lu from endpoints, %lu from routers",
			st->endpoint_sent_packet, st->router_sent_packet);
	printf("\n");
}

void emu_save_admission_stats() {
	memcpy(&emu_saved_admission_statistics, &g_emu_state.stat,
			sizeof(emu_saved_admission_statistics));
}

void emu_save_admission_core_stats(int i) {
	// TODO: copy admission core stats, once they are used
}
