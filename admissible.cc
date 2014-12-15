/*
 * admissible.cc
 *
 *  Created on: December 15, 2014
 *      Author: aousterh
 */

/* dummy struct declaration */
struct admissible_state;

#include "../graph-algo/admissible.h"
#include "emulation.h"

void add_backlog(struct admissible_state *state, uint16_t src, uint16_t dst,
		uint32_t amount) {
	uint16_t dst_node;
	uint8_t flow;
	Emulation *emu_state = (Emulation *) state;

	flow = dst & FLOW_MASK;
	dst_node = dst >> FLOW_SHIFT;
	emu_state->add_backlog(src, dst_node, flow, amount);
}

void reset_sender(struct admissible_state *state, uint16_t src)
{
	Emulation *emu_state = (Emulation *) state;
	emu_state->reset_sender(src);
}
