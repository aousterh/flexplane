/*
 * port.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef PORT_H_
#define PORT_H_

#include "../graph-algo/fp_ring.h"

#include <assert.h>
#include <stdlib.h>

/**
 * A port in the emulated network.
 * @q_ingress: a queue of packets arriving at this port
 * @q_egress: a queue of packets departing this port
 */
struct emu_port {
	struct fp_ring			*q_ingress;
	struct fp_ring			*q_egress;
};

/**
 * Initialize a pair of connected ports by assigning their shared queues.
 * @port_top: the port at the top of the link
 * @port_bottom: the port at the bottom of the link
 * @q_up: the queue of packets from the bottom port to the top port
 * @q_down: the queue of packets from the top port to the bottom port
 * @return 0 on success, negative value on error.
 */
static inline
int port_pair_init(struct emu_port *port_top, struct emu_port *port_bottom,
		   struct fp_ring *q_up, struct fp_ring *q_down)
{
	assert(port_top != NULL);
	assert(port_bottom != NULL);
	assert(q_up != NULL);
	assert(q_down != NULL);

	port_top->q_ingress = q_up;
	port_top->q_egress = q_down;
	port_bottom->q_ingress = q_down;
	port_bottom->q_egress = q_up;

	return 0;
}

#endif /* PORT_H_ */
