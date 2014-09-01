/*
 * drop_tail_router.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_ROUTER_H_
#define DROP_TAIL_ROUTER_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"

struct packet_queue;

/**
 * State maintained by each drop tail router.
 * @output_queue: a queue of packets for each output port
 */
struct drop_tail_router {
	struct packet_queue output_queue[EMU_ROUTER_NUM_PORTS];
};

/**
 * Initialize a router.
 * @return 0 on success, negative value on error
 */
int drop_tail_router_init(struct emu_router *rtr);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void drop_tail_router_cleanup(struct emu_router *rtr);

/**
 * Emulate one timeslot at a given router. For now, assume that routers
 * can process MTUs with no additional delay beyond the queueing delay.
 */
void drop_tail_router_emulate(struct emu_router *rtr);

static struct router_ops drop_tail_router_ops = {
		.priv_size	= sizeof(struct drop_tail_router),
		.init		= &drop_tail_router_init,
		.cleanup	= &drop_tail_router_cleanup,
		.emulate	= &drop_tail_router_emulate,
};

#endif /* DROP_TAIL_ROUTER_H_ */
