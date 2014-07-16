/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

struct emu_router;
struct fp_mempool;

/**
 * A representation of an endpoint (server) in the emulated network.
 */
struct emu_endpoint {
	uint16_t id;
	struct fp_ring *q_in;
	struct emu_router *router;
};

/**
 * Add backlog to dst at this endpoint.
 */
void endpoint_add_backlog(struct emu_endpoint *endpoint,
			  struct fp_mempool *packet_mempool, uint16_t dst,
			  uint32_t amount, uint16_t start_id);

/**
 * Emulate one timeslot at a given endpoint.
 */
void endpoint_emulate_timeslot(struct emu_endpoint *endpoint);

/**
 * Reset the state of a single endpoint.
 */
void endpoint_reset_state(struct emu_endpoint *endpoint,
			  struct fp_mempool * packet_mempool);

/**
 * Initialize the state of a single endpoint.
 */
void endpoint_init_state(struct emu_endpoint *endpoint, uint16_t id,
			 struct fp_ring *q_in, struct emu_router *router);

#endif /* ENDPOINT_H_ */
