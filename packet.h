/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <inttypes.h>

struct emu_state;

/**
 * A representation of an MTU-sized packet in the emulated network.
 * @src: the id of the source endpoint in the emulation
 * @dst: the id of the destination endpoint in the emulation
 * @state: the global emulation state (to enable freeing of this packet)
 */
struct emu_packet {
	uint16_t			src;
	uint16_t			dst;
	struct emu_state	*state;
};

/**
 * Initialize a packet.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
				 struct emu_state *state) {
	packet->src = src;
	packet->dst = dst;
	packet->state = state;
}

/**
 * Add a dropped demand to the 'admitted' list to be passed to the comm cores.
 * Used as a helper function or when allocating memory for a packet fails.
 */
void drop_demand(struct emu_state *state, uint16_t src, uint16_t dst);

#endif /* PACKET_H_ */
