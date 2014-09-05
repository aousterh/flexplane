/*
 * drop_tail.c
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"
#include "api_impl.h"
#include "port.h"

#define DROP_TAIL_PORT_CAPACITY 5

int drop_tail_router_init(struct emu_router *rtr, void *args) {
	struct drop_tail_router *rtr_priv;
	struct drop_tail_args *drop_tail_args;
	uint32_t i, port_capacity;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* use args if supplied, otherwise use defaults */
	if (args != NULL) {
		drop_tail_args = (struct drop_tail_args *) args;
		port_capacity = drop_tail_args->port_capacity;
	} else
		port_capacity = DROP_TAIL_PORT_CAPACITY;

	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++)
		queue_create(&rtr_priv->output_queue[i], port_capacity);

	return 0;
}

void drop_tail_router_cleanup(struct emu_router *rtr) {
	struct drop_tail_router *rtr_priv;
	uint16_t i;
	struct emu_packet *packet;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* free all queued packets */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		while (queue_dequeue(&rtr_priv->output_queue[i], &packet) == 0)
			free_packet(packet);
	}
}

void drop_tail_router_emulate(struct emu_router *rtr) {
	struct drop_tail_router *rtr_priv;
	uint16_t i;
	struct emu_port *port;
	struct emu_packet *packet;
	struct packet_queue *output_q;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* try to transmit one packet per output port */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		output_q = &rtr_priv->output_queue[i];

		/* dequeue one packet for this port, send it */
		if (queue_dequeue(output_q, &packet) == 0) {
			port = router_port(rtr, i);
			send_packet(port, packet);
		}
	}

	/* move packets from the input ports to the output queues */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		port = router_port(rtr, i);

		/* try to receive all packets from this port */
		while ((packet = receive_packet(port)) != NULL) {
			output_q = &rtr_priv->output_queue[packet->dst];

			if (queue_enqueue(output_q, packet) != 0) {
				/* no space to enqueue, drop this packet */
				drop_packet(packet);
				continue;
			}
		}
	}
}

int drop_tail_endpoint_init(struct emu_endpoint *ep, void *args) {
	return 0;
};

void drop_tail_endpoint_reset(struct emu_endpoint *ep) {};

void drop_tail_endpoint_cleanup(struct emu_endpoint *ep) {};

void drop_tail_endpoint_emulate(struct emu_endpoint *ep) {
	struct emu_packet *packet;
	struct emu_port *port;

	/* try to dequeue one packet from network stack */
	packet = dequeue_packet_at_endpoint(ep);
	port = endpoint_port(ep);
	if (packet != NULL) {
		/* try to transmit the packet to the next router */
		send_packet(port, packet);
	}

	/* pass all incoming packets up the network stack */
	while ((packet = receive_packet(port)) != NULL) {
		enqueue_packet_at_endpoint(ep, packet);
	}
}
