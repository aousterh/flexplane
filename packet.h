/*
 * packet.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef PACKET_H_
#define PACKET_H_

#define EMU_PKT_QUEUE_SIZE 100

/**
 * A representation of an MTU-sized packet in the emulated network.
 */
struct emu_packet {
        uint16_t src;
        uint16_t dst;
        uint32_t id;
};

#endif /* PACKET_H_ */
