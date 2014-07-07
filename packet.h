/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <inttypes.h>
#include <stdio.h>

#define EMU_PKT_QUEUE_SIZE 100

/**
 * A representation of an MTU-sized packet in the emulated network.
 */
struct emu_packet {
        uint16_t src;
        uint16_t dst;
        uint16_t id;
};

/**
 * Initialize a packet.
 */
static inline
void packet_init(struct emu_packet *packet, uint16_t src, uint16_t dst,
                 uint16_t id) {
        packet->src = src;
        packet->dst = dst;
        packet->id = id;
}

#endif /* PACKET_H_ */
