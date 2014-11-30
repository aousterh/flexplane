/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <inttypes.h>


/**
 * A representation of an MTU-sized packet in the emulated network.
 * @src: the id of the source endpoint in the emulation
 * @dst: the id of the destination endpoint in the emulation
 * @flow: id to disambiguate between flows with the same src and dst ips
 */
struct emu_packet {
	uint16_t	src;
	uint16_t	dst;
	uint8_t		flow;
};

/**
 * Initialize a packet.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
		uint16_t flow) {
	packet->src = src;
	packet->dst = dst;
	packet->flow = flow;
}

/**
 * Add a dropped demand to the 'admitted' list to be passed to the comm cores.
 * Used as a helper function or when allocating memory for a packet fails.
 */
void drop_demand(uint16_t src, uint16_t dst, uint16_t flow);

#endif /* PACKET_H_ */
