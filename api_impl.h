/*
 * api_impl.h
 *
 * The implementations of static inline functions declared in api.h
 *
 *  Created on: September 3, 2014
 *      Author: aousterh
 */

#ifndef API_IMPL_H__
#define API_IMPL_H__

#include "admitted.h"
#include "api.h"
#include "packet.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "output.h"

#include <assert.h>

static inline
struct emu_packet *create_packet(uint16_t src, uint16_t dst, uint16_t flow) {
	struct emu_packet *packet;

	/* allocate a packet */
	while (fp_mempool_get(g_state->packet_mempool, (void **) &packet)
	       == -ENOENT) {
		adm_log_emu_packet_alloc_failed(&g_state->stat);
	}
	packet_init(packet, src, dst, flow);

	return packet;
}

static inline
void free_packet(struct emu_packet *packet) {
	/* return the packet to the mempool */
	fp_mempool_put(g_state->packet_mempool, packet);
}

static inline
void *packet_priv(struct emu_packet *packet) {
	return (char *) packet + EMU_ALIGN(sizeof(struct emu_packet));
}

#endif /* API_IMPL_H__ */
