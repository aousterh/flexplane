/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "../protocol/flags.h"
#include <inttypes.h>

/* alignment macros based on /net/pkt_sched.h */
#define EMU_ALIGNTO				64
#define EMU_ALIGN(len)			(((len) + EMU_ALIGNTO-1) & ~(EMU_ALIGNTO-1))

/**
 * A representation of an MTU-sized packet in the emulated network.
 * @src: the id of the source endpoint in the emulation
 * @dst: the id of the destination endpoint in the emulation
 * @flow: id to disambiguate between flows with the same src and dst ips
 * @id: sequential id within this flow, to enforce ordering of packets
 * @flags: flags indicating marks or other info to be conveyed to endpoints
 */
struct emu_packet {
	uint16_t	src;
	uint16_t	dst;
	uint16_t	flow;
	uint16_t	id;
	uint16_t	flags;

	/* custom per-scheme fields */
	uint32_t	priority;
}  __attribute__((aligned(64))) /* don't want sharing between cores */;

/**
 * Initialize a packet with @src, @dst, @flow, and @id. @areq_data provides
 * additional information in an array of bytes. The use of this data varies by
 * emulated scheme.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
		uint16_t flow, uint16_t id, uint8_t *areq_data);

/**
 * Frees a packet.
 */
static inline
void free_packet(struct emu_packet *packet, struct fp_mempool *packet_mempool);

/**
 * Frees a batch of packets.
 */
static inline
void free_packet_bulk(struct emu_packet **packets,
		struct fp_mempool *packet_mempool, uint32_t n);

/**
 * Frees all the packets in an fp_ring, and frees the ring itself.
 */
static inline
void free_packet_ring(struct fp_ring *packet_ring,
		struct fp_mempool *packet_mempool);

/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *packet_priv(struct emu_packet *packet);

#endif /* PACKET_H_ */
