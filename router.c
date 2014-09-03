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
#include "../graph-algo/platform.h"
#include "assert.h"


/**
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

struct emu_port *router_port(struct emu_router *rtr, uint32_t port_ind) {
	return &rtr->ports[port_ind];
}

uint32_t router_id(struct emu_router *rtr) {
	return rtr->id;
}


/**
 * Functions declared in router.h, used only within the framework
 */

int router_init(struct emu_router *rtr, uint16_t id, struct emu_ops *ops) {
	uint16_t i;

	assert(rtr != NULL);
	assert(ops != NULL);

	rtr->id = id;
	rtr->ops = &ops->rtr_ops;

	return rtr->ops->init(rtr, ops->args);
}

void router_cleanup(struct emu_router *rtr) {
	uint16_t i;
	struct emu_packet *packet;

	rtr->ops->cleanup(rtr);

	/* free egress queues in ports (ingress will be freed by other port) */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		while (fp_ring_dequeue(rtr->ports[i].q_egress, (void **) &packet) == 0) {
			free_packet(packet);
		}
		fp_free(rtr->ports[i].q_egress);
	}
}

void router_emulate(struct emu_router *rtr) {
	rtr->ops->emulate(rtr);
}
