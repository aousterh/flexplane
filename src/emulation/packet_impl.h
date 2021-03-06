/*
 * packet_impl.h
 *
 * The implementations of static inline functions declared in packet.h
 *
 *  Created on: September 3, 2014
 *      Author: aousterh
 */

#ifndef PACKET_IMPL_H__
#define PACKET_IMPL_H__

#include "packet.h"
#include "admissible_log.h"
#include "emulation.h"

static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
		uint16_t flow, uint16_t id, uint8_t *areq_data) {
	(void) areq_data;

	packet->src = src;
	packet->dst = dst;
	packet->flow = flow;
	packet->id = id;
	packet->flags = EMU_FLAGS_NONE; /* start with no flags */

	/* copy algo-specific fields to emu_packet */
#if defined(PFABRIC)
        if (areq_data != NULL)
          packet->priority = ntohl(*((uint32_t *) areq_data));
        else
          packet->priority = 0;
#endif

#if defined(USE_TSO)
        packet->n_mtus = areq_data[0];
#endif

#if defined(LSTF)
        if (areq_data != NULL){
                packet->slack = (((uint64_t)(ntohl(*((uint32_t *) areq_data)))) << 32) + ntohl(*((uint32_t *) areq_data + 1));
        }
        else{
                packet->slack = 0;
        }
#endif
}

static inline
void free_packet(struct emu_packet *packet, struct fp_mempool *packet_mempool)
{
	/* return the packet to the mempool */
	fp_mempool_put(packet_mempool, packet);
}

static inline
void free_packet_bulk(struct emu_packet **packets,
		struct fp_mempool *packet_mempool, uint32_t n)
{
	/* return the packet to the mempool */
	fp_mempool_put_bulk(packet_mempool, (void **) packets, n);
}

static inline
void free_packet_ring(struct fp_ring *packet_ring,
		struct fp_mempool *packet_mempool)
{
	struct emu_packet *packet;

	while (fp_ring_dequeue(packet_ring, (void **) &packet) == 0) {
		free_packet(packet, packet_mempool);
	}
	fp_free(packet_ring);
}

static inline
void *packet_priv(struct emu_packet *packet) {
	return (char *) packet + EMU_ALIGN(sizeof(struct emu_packet));
}

#endif /* API_IMPL_H__ */
