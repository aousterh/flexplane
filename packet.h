/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "../protocol/flags.h"
#include <inttypes.h>

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
};

/**
 * Initialize a packet with @src, @dst, @flow, and @id. @areq_data provides
 * additional information in an array of bytes. The use of this data varies by
 * emulated scheme.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
		uint16_t flow, uint16_t id, uint8_t *areq_data) {
	packet->src = src;
	packet->dst = dst;
	packet->flow = flow;
	packet->id = id;
	packet->flags = EMU_FLAGS_NONE; /* start with no flags */

	/* TODO: add algo-specific fields to emu_packet and populate them from
	 * areq_data, based on the algorithm used. */
}

#endif /* PACKET_H_ */
