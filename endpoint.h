/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "../graph-algo/fp_ring.h"

struct emu_port;
struct endpoint_ops;
struct emu_ops;

/**
 * A representation of an endpoint (server) in the emulated network.
 * @id: the unique id of this endpoint
 * @q_egress: the queue of outgoing packets
 * @port: the egress port of this endpoint
 * @state: the global emulation state
 * @ops: endpoint functions implemented by the emulation algorithm
 */
struct emu_endpoint {
	uint16_t			id;
	struct fp_ring		*q_egress;
	struct emu_port		*port;
	struct endpoint_ops	*ops;
};

/**
 * Initialize an endpoint.
 * @return 0 on success, negative value on error.
 */
int endpoint_init(struct emu_endpoint *ep, uint16_t id,
			struct fp_ring *q_egress, struct emu_port *port,
			struct emu_ops *ops);

/**
 * Reset an endpoint. This happens when endpoints lose sync with the
 * arbiter. To resync, a reset occurs, then backlogs are re-added based
 * on endpoint reports.
 */
void endpoint_reset(struct emu_endpoint *ep);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void endpoint_cleanup(struct emu_endpoint *ep);

/**
 * Emulate one timeslot at a given endpoint.
 */
void endpoint_emulate(struct emu_endpoint *ep);

/**
 * Add backlog to dst at this endpoint.
 */
void endpoint_add_backlog(struct emu_endpoint *ep, uint16_t dst,
						  uint32_t amount);

#endif /* ENDPOINT_H_ */
