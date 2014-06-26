/*
 * switch.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef SWITCH_H_
#define SWITCH_H_

#include "packet.h"
#include "topology.h"

struct emu_endpoint;
struct emu_switch;
struct fp_ring;

/**
 * An output queue to an endpoint in the emulated network.
 */
struct emu_endpoint_output {
        struct fp_ring *q_out;
};

/**
 * An output queue to a switch in the emulated network.
 */
struct emu_switch_output {
        struct fp_ring *q_out;
        struct emu_switch *adj_switch;
};

/**
 * A representation of a switch in the emulated network. For now, there is a
 * fixed capacity for each output queue.
 */
struct emu_switch {
        struct fp_ring *q_in;
        struct emu_endpoint_output endpoint_outputs[EMU_SWITCH_MAX_ENDPOINT_PORTS];
        struct emu_switch_output switch_outputs[EMU_SWITCH_MAX_SWITCH_PORTS];
};

#endif /* SWITCH_H_ */
