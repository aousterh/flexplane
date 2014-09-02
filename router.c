/*
 * router.c
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "api.h"
#include "port.h"
#include "../graph-algo/admissible_algo_log.h"
#include "assert.h"


/**
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

struct emu_port *router_ingress_port(struct emu_router *rtr,
		uint32_t port_ind) {
	return rtr->input_ports[port_ind];
}

struct emu_port *router_egress_port(struct emu_router *rtr,
		uint32_t port_ind) {
	return rtr->output_ports[port_ind];
}

uint32_t router_id(struct emu_router *rtr) {
	return rtr->id;
}


/**
 * Functions declared in router.h, used only within the framework
 */

int router_init(struct emu_router *rtr, uint16_t id, struct emu_port *ports,
				struct emu_state *state, struct emu_ops *ops) {
	uint16_t i;

	assert(rtr != NULL);
	assert(ports != NULL);
	assert(state != NULL);
	assert(ops != NULL);

	rtr->id = id;
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		rtr->input_ports[i] = &ports[i];
		rtr->output_ports[i] = &ports[EMU_ROUTER_NUM_PORTS + i];
	}
	rtr->state = state;
	rtr->ops = &ops->rtr_ops;

	return rtr->ops->init(rtr, ops->args);
}

void router_cleanup(struct emu_router *rtr) {
	rtr->ops->cleanup(rtr);
}

void router_emulate(struct emu_router *rtr) {
	rtr->ops->emulate(rtr);
}
