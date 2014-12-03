/*
 * drop_tail.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_H_
#define DROP_TAIL_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"

struct packet_queue;

struct drop_tail_args {
	uint16_t port_capacity;
};

/**
 * Output queues for a give input port.
 * @output: one queue per output port
 */
struct input_queues {
	struct packet_queue output[EMU_ROUTER_NUM_PORTS];
};

/**
 * State maintained by each drop tail router.
 * @input: a set of input queues for each incoming port
 * @next_input: for each output port, the next input to send from
 * 		(should be taken mod EMU_ROUTER_NUM_PORTS)
 */
struct drop_tail_router {
	struct input_queues input[EMU_ROUTER_NUM_PORTS];

	/* per-output state */
	uint16_t next_input[EMU_ROUTER_NUM_PORTS];
	uint32_t non_empty_inputs[EMU_ROUTER_NUM_PORTS];
};

/**
 * Initialize a router.
 * @return 0 on success, negative value on error
 */
int drop_tail_router_init(struct emu_router *rtr, void *args);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void drop_tail_router_cleanup(struct emu_router *rtr);

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void drop_tail_router_emulate(struct emu_router *rtr);

/**
 * Initialize an endpoint.
 * @return 0 on success, negative value on error.
 */
int drop_tail_endpoint_init(struct emu_endpoint *ep, void *args);

/**
 * Reset an endpoint. This happens when endpoints lose sync with the
 * arbiter. To resync, a reset occurs, then backlogs are re-added based
 * on endpoint reports.
 */
void drop_tail_endpoint_reset(struct emu_endpoint *ep);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void drop_tail_endpoint_cleanup(struct emu_endpoint *ep);

/**
 * Emulate one timeslot at a given endpoint.
 */
void drop_tail_endpoint_emulate(struct emu_endpoint *endpoint);

/**
 * Drop tail functions and parameters.
 */
static struct emu_ops drop_tail_ops = {
		.rtr_ops = {
				.priv_size	= sizeof(struct drop_tail_router),
				.init		= &drop_tail_router_init,
				.cleanup	= &drop_tail_router_cleanup,
				.emulate	= &drop_tail_router_emulate,
		},
		.ep_ops = {
				.priv_size	= 0, /* no private state necessary */
				.init		= &drop_tail_endpoint_init,
				.reset		= &drop_tail_endpoint_reset,
				.cleanup	= &drop_tail_endpoint_cleanup,
				.emulate	= &drop_tail_endpoint_emulate,
		},
		.packet_priv_size	=	0,
		.args				= NULL,
};

#endif /* DROP_TAIL_H_ */
