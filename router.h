/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "config.h"
#include "../graph-algo/fp_ring.h"
#include <inttypes.h>

struct router_ops;
struct emu_ops;

/**
 * A representation of a router in the emulated network.
 * @id: the unique id of this router
 * @q_ingress: queue of incoming packets
 * @ops: router functions implemented by the emulation algorithm
 */
struct emu_router {
	uint16_t			id;
	struct fp_ring		*q_ingress;
	struct router_ops	*ops;
};

/**
 * Initialize a router.
 * @return 0 on success, negative value on error
 */
int router_init(struct emu_router *rtr, uint16_t id, struct emu_ops *ops,
		struct fp_ring *q_ingress);

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
