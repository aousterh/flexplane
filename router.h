/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "packet.h"
#include "topology.h"

struct emu_endpoint;
struct emu_router;
struct fp_ring;

/**
 * An output queue to an endpoint in the emulated network.
 */
struct emu_endpoint_output {
        struct fp_ring *q_out;
};

/**
 * An output queue to a router in the emulated network.
 */
struct emu_router_output {
        struct fp_ring *q_out;
        struct emu_router *router;
};

/**
 * A representation of a router in the emulated network. For now, there is a
 * fixed capacity for each output queue.
 */
struct emu_router {
        struct fp_ring *q_in;
        struct emu_endpoint_output endpoint_outputs[EMU_ROUTER_MAX_ENDPOINT_PORTS];
        struct emu_router_output router_outputs[EMU_ROUTER_MAX_ROUTER_PORTS];
};

#endif /* ROUTER_H_ */
