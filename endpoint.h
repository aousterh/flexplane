/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

struct emu_switch;
struct fp_ring;

/**
 * A representation of an endpoint (server) in the emulated network.
 */
struct emu_endpoint {
        struct fp_ring *q_packet_in;
        struct emu_switch *adj_switch;
};

#endif /* ENDPOINT_H_ */
