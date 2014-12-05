/*
 * drop_tail.c
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"
#include "api.h"
#include "api_impl.h"

#define DROP_TAIL_PORT_CAPACITY 5

int drop_tail_router_init(struct emu_router *rtr, void *args) {
	struct drop_tail_router *rtr_priv;
	struct drop_tail_args *drop_tail_args;
	uint16_t i, j, port_capacity;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* use args if supplied, otherwise use defaults */
	if (args != NULL) {
		drop_tail_args = (struct drop_tail_args *) args;
		port_capacity = drop_tail_args->port_capacity;
	} else
		port_capacity = DROP_TAIL_PORT_CAPACITY;

	/* initialize packet queues */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		for (j = 0; j < EMU_ROUTER_NUM_PORTS; j++)
			queue_create(&rtr_priv->input[i].output[j], port_capacity);
	}

	/* initialize per-output state */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		rtr_priv->next_input[i] = 0;
		rtr_priv->non_empty_inputs[i] = 0;
	}

	return 0;
}

void drop_tail_router_cleanup(struct emu_router *rtr) {
	struct drop_tail_router *rtr_priv;
	uint16_t i, j;
	struct emu_packet *packet;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* free all queued packets */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		for (j = 0; j < EMU_ROUTER_NUM_PORTS; j++) {
			while (queue_dequeue(&rtr_priv->input[i].output[j], &packet) == 0)
				free_packet(packet);
		}
	}
}

/**
 * Shift value to the right by amount, place bits shifted-off on the left side
 * again. Return the shifted result.
 */
static inline
uint32_t circular_shift_right(uint32_t value, uint32_t amount) {
	return (value >> amount) | (value << (32 - amount));
}

void drop_tail_router_emulate(struct emu_router *rtr) {
	struct drop_tail_router *rtr_priv;
	uint16_t output, next_input, input;
	int ret;
	uint32_t input_bitmask, offset;
	struct emu_port *port;
	struct emu_packet *packet;
	struct packet_queue *packet_q;

	/* get private state for this router */
	rtr_priv = (struct drop_tail_router *) router_priv(rtr);

	/* try to transmit one packet per output port, in a round-robin manner */
	for (output = 0; output < EMU_ROUTER_NUM_PORTS; output++) {
		input_bitmask = rtr_priv->non_empty_inputs[output];

		if (input_bitmask == 0)
			continue; /* no queued packets for this output */

		/* find the first set bit, indicating the next input to serve */
		asm("bsfl %1,%0" : "=r"(offset) : "r"(input_bitmask));

		/* the first bit in the bitmask corresponds to the input after the last
		 * input served by this output. */
		next_input = (rtr_priv->next_input[output] + offset) % EMU_ROUTER_NUM_PORTS;
		packet_q = &rtr_priv->input[next_input].output[output];

		/* dequeue one packet for this port, send it */
		ret = queue_dequeue(packet_q, &packet);
		assert(ret == 0); /* queue must have a packet */
		port = router_port(rtr, output);
		adm_log_emu_router_sent_packet(&g_state->stat);
		send_packet(port, packet);

		/* mark this queue as empty, if necessary */
		if (queue_empty(packet_q))
			rtr_priv->non_empty_inputs[output] &= ~(0x1UL << offset);

		/* update state for this output */
		rtr_priv->next_input[output] = next_input + 1;
		/* circular shift by offset + 1 */
		rtr_priv->non_empty_inputs[output] =
				circular_shift_right(rtr_priv->non_empty_inputs[output],
						offset + 1);
	}

	/* move packets from the input ports to the correct queues */
	for (input = 0; input < EMU_ROUTER_NUM_PORTS; input++) {
		port = router_port(rtr, input);

		/* try to receive all packets from this port */
		while ((packet = receive_packet(port)) != NULL) {
			packet_q = &rtr_priv->input[input].output[packet->dst];

			/* mark that this output has a pending packet */
			next_input = rtr_priv->next_input[packet->dst];
			rtr_priv->non_empty_inputs[packet->dst] |= (1 << (input-next_input));

			if (queue_enqueue(packet_q, packet) != 0) {
				/* no space to enqueue, drop this packet */
				adm_log_emu_router_dropped_packet(&g_state->stat);
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
		adm_log_emu_endpoint_sent_packet(&g_state->stat);
		send_packet(port, packet);
	}

	/* pass all incoming packets up the network stack */
	while ((packet = receive_packet(port)) != NULL) {
		enqueue_packet_at_endpoint(ep, packet);
	}
}
