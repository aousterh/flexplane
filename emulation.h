/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "endpoint.h"
#include "switch.h"
#include "topology.h"

/**
 * Data structure to store the state of the emulation
 */
struct emu_state {
        struct emu_endpoint endpoints[EMU_NUM_ENDPOINTS];
        struct emu_switch tors[EMU_NUM_TORS];
};

/**
 * Add backlog from src to dst
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
                     uint32_t amount, uint16_t start_id);

/**
 * Emulate a single timeslot
 */
void emu_timeslot(struct emu_state *state);

#endif /* EMULATION_H_ */
