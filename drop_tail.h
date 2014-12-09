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
 * State maintained by each drop tail router.
 * @output_queue: a queue of packets for each output port
 */
struct drop_tail_router {
	struct packet_queue output_queue[EMU_ROUTER_NUM_PORTS];
};

/**
 * State maintained by each drop tail endpoint.
 * @output_queue: a queue of packets to be sent out
 */
struct drop_tail_endpoint {
	struct packet_queue output_queue;
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
 * Process a packet arriving at the router.
 */
void drop_tail_router_receive(struct emu_router *rtr, struct emu_packet *p);

/**
 * Return a packet to send from the router on output port.
 */
void drop_tail_router_send(struct emu_router *rtr, uint16_t output,
		struct emu_packet **packet);

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
 * Receive a packet @p from the application at endpoint @ep.
 */
void drop_tail_endpoint_rcv_from_app(struct emu_endpoint *ep,
		struct emu_packet *p);

/**
 * Receive a packet @p from the network at endpoint @ep.
 */
void drop_tail_endpoint_rcv_from_net(struct emu_endpoint *ep,
		struct emu_packet *p);

/**
 * Choose a packet @p to send on the network at endpoint @ep.
 */
void drop_tail_endpoint_send_to_net(struct emu_endpoint *ep,
		struct emu_packet **p);

/**
 * Drop tail functions and parameters.
 */
static struct emu_ops drop_tail_ops = {
		.rtr_ops = {
				.priv_size	= sizeof(struct drop_tail_router),
				.init		= &drop_tail_router_init,
				.cleanup	= &drop_tail_router_cleanup,
				.send		= &drop_tail_router_send,
				.receive	= &drop_tail_router_receive,
		},
		.ep_ops = {
				.priv_size		= sizeof(struct drop_tail_endpoint),
				.init			= &drop_tail_endpoint_init,
				.reset			= &drop_tail_endpoint_reset,
				.cleanup		= &drop_tail_endpoint_cleanup,
				.rcv_from_app	= &drop_tail_endpoint_rcv_from_app,
				.rcv_from_net	= &drop_tail_endpoint_rcv_from_net,
				.send_to_net	= &drop_tail_endpoint_send_to_net,
		},
		.packet_priv_size	=	0,
		.args				= NULL,
};

#endif /* DROP_TAIL_H_ */
