/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"
#include "api.h"
#include "api_impl.h"

#define DROP_TAIL_PORT_CAPACITY 128

DropTailRouter::DropTailRouter(uint16_t id, struct fp_ring *q_ingress,
		struct drop_tail_args *args) : Router(id, q_ingress) {
	uint32_t i, port_capacity;

	/* use args if supplied, otherwise use defaults */
	if (args != NULL) {
		port_capacity = args->port_capacity;
	} else
		port_capacity = DROP_TAIL_PORT_CAPACITY;

	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++)
		queue_create(&this->output_queue[i], port_capacity);
}

DropTailRouter::~DropTailRouter() {
	uint16_t i;
	struct emu_packet *packet;

	/* free all queued packets */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		while (queue_dequeue(&this->output_queue[i], &packet) == 0)
			free_packet(packet);
	}
}

void DropTailRouter::push(struct emu_packet *p) {
	uint16_t output;
	struct packet_queue *output_q;

	output = get_output_queue(this, p);
	output_q = &this->output_queue[output];

	/* try to enqueue the packet */
	if (queue_enqueue(output_q, p) != 0) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_router_dropped_packet(&g_state->stat);
		drop_packet(p);
	}
}

void DropTailRouter::pull(uint16_t output, struct emu_packet **packet) {
	/* dequeue one packet for this output if the queue is non-empty */
	*packet = NULL;
	queue_dequeue(&this->output_queue[output], packet);
}


DropTailEndpoint::DropTailEndpoint(uint16_t id, struct drop_tail_args *args) :
		Endpoint(id) {
	uint32_t port_capacity;

	/* use args if supplied, otherwise use defaults */
	if (args != NULL)
		port_capacity = args->port_capacity;
	else
		port_capacity = DROP_TAIL_PORT_CAPACITY;

	queue_create(&output_queue, port_capacity);
}

DropTailEndpoint::~DropTailEndpoint() {
	reset();
}

void DropTailEndpoint::reset() {
	struct emu_packet *packet;

	/* dequeue all queued packets */
	while (queue_dequeue(&output_queue, &packet) == 0)
		free_packet(packet);
}

void DropTailEndpoint::new_packet(struct emu_packet *packet) {
	/* try to enqueue the packet */
	if (queue_enqueue(&output_queue, packet) != 0) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_endpoint_dropped_packet(&g_state->stat);
		drop_packet(packet);
	}
}

void DropTailEndpoint::push(struct emu_packet *packet) {
	assert(packet->dst == id);

	/* pass the packet up the stack */
	enqueue_packet_at_endpoint(packet);
}

void DropTailEndpoint::pull(struct emu_packet **packet) {
	/* dequeue one incoming packet if the queue is non-empty */
	*packet = NULL;
	queue_dequeue(&output_queue, packet);
}
