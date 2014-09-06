/*
 * packet.c
 *
 *  Created on: September 1, 2014
 *      Author: aousterh
 */

#include "packet.h"
#include "api.h"
#include "admitted.h"
#include "emulation.h"
#include "admissible_log.h"
#include "../graph-algo/platform.h"

void drop_demand(uint16_t src, uint16_t dst) {
	/* this packet should be dropped */
	admitted_insert_dropped_edge(g_state->admitted, src, dst);
	adm_log_emu_dropped_demand(&g_state->stat);
}

struct emu_packet *create_packet(uint16_t src, uint16_t dst) {
	struct emu_packet *packet;

	/* allocate a packet */
	if (fp_mempool_get(g_state->packet_mempool, (void **) &packet)
	       == -ENOENT) {
		adm_log_emu_packet_alloc_failed(&g_state->stat);
		return NULL;
	}
	packet_init(packet, src, dst);

	return packet;
}
