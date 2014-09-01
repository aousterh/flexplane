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

/**
 * Returns a pointer to the ingress port with index port_ind at router rtr.
 */
struct emu_port *router_ingress_port(struct emu_router *rtr,
		uint32_t port_ind) {
	return rtr->input_ports[port_ind];
}

/**
 * Returns a pointer to the egress port with index port_ind at router rtr.
 */
struct emu_port *router_egress_port(struct emu_router *rtr,
		uint32_t port_ind) {
	return rtr->output_ports[port_ind];
}

/**
 * Returns the id of router rtr.
 */
uint32_t router_id(struct emu_router *rtr) {
	return rtr->id;
}


/**
 * Functions declared in router.h, used only within the framework
 */

/**
 * Initialize a router.
 * @ports: an array of ports for this router (input, then output)
 * @return 0 on success, negative value on error
 */
int router_init(struct emu_router *rtr, uint16_t id, struct emu_port *ports,
				struct emu_state *state, struct router_ops *ops) {
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
	rtr->ops = ops;

	return rtr->ops->init(rtr);
}

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void router_cleanup(struct emu_router *rtr) {
	rtr->ops->cleanup(rtr);
}

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void router_emulate(struct emu_router *rtr) {
	rtr->ops->emulate(rtr);
}
