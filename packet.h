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
 * @id: sequential id within this flow, to enforce ordering of packets
 */
struct emu_packet {
	uint16_t	src;
	uint16_t	dst;
	uint16_t	flow;
	uint16_t	id;
};

/**
 * Initialize a packet.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
		uint16_t flow, uint16_t id) {
	packet->src = src;
	packet->dst = dst;
	packet->flow = flow;
	packet->id = id;
}

#endif /* PACKET_H_ */
