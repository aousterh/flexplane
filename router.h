/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "config.h"
#include <inttypes.h>

struct emu_port;
struct router_ops;
struct emu_ops;

/**
 * A representation of a router in the emulated network.
 * @id: the unique id of this router
 * @input_ports: ports that deliver packets to this router
 * @output_ports: ports this router may transmit packets out of
 * @state: the global emulation state
 * @ops: router functions implemented by the emulation algorithm
 */
struct emu_router {
	uint16_t			id;
	struct emu_port		*input_ports[EMU_ROUTER_NUM_PORTS];
	struct emu_port		*output_ports[EMU_ROUTER_NUM_PORTS];
	struct router_ops	*ops;
};

/**
 * Initialize a router.
 * @return 0 on success, negative value on error
 */
int router_init(struct emu_router *rtr, uint16_t id, struct emu_port *ports,
				struct emu_ops *ops);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void router_cleanup(struct emu_router *rtr);

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void router_emulate(struct emu_router *rtr);

#endif /* ROUTER_H_ */
