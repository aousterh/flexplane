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
#define EMU_NO_DROP        0
#define EMU_DROP           1

/**
 * A representation of an MTU-sized packet in the emulated network.
 */
struct emu_packet {
        uint16_t src;
        uint16_t dst;
        uint32_t id;
        uint8_t should_drop;
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
        packet->should_drop = EMU_NO_DROP;
}

/**
 * Mark a packet as dropped.
 */
static inline
void packet_mark_as_dropped(struct emu_packet *packet) {
        packet->should_drop = EMU_DROP;
}

/**
 * Print out packet info.
 */
static inline
void packet_print(struct emu_packet *packet) {
        if (packet->should_drop) {
                printf("\tDROP src %d to dst %d (id %d)\n", packet->src,
                       packet->dst, packet->id);
        } else {
                printf("\tsrc %d to dst %d (id %d)\n", packet->src,
                       packet->dst, packet->id);
        }
}

#endif /* PACKET_H_ */
