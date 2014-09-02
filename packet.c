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
#include "../graph-algo/admissible_algo_log.h"
#include "../graph-algo/platform.h"

/*
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

void drop_demand(uint16_t src, uint16_t dst) {
	/* this packet should be dropped */
	admitted_insert_dropped_edge(g_state->admitted, src, dst);

	#ifdef AUTO_RE_REQUEST_BACKLOG
	/* backlog for dropped packets will not be re-requested,
	 * so automatically request the backlog again */
	emu_add_backlog(state, src, dst, 1);
	#endif
}

void drop_packet(struct emu_packet *packet) {
	drop_demand(packet->src, packet->dst);

	free_packet(packet);
}

void free_packet(struct emu_packet *packet) {
	/* return the packet to the mempool */
	fp_mempool_put(g_state->packet_mempool, packet);
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
