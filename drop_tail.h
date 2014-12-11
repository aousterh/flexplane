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
#include "router.h"
#include "endpoint.h"
#include "endpoint_group.h"

struct packet_queue;

struct drop_tail_args {
	uint16_t port_capacity;
};

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public Router {
public:
	DropTailRouter(uint16_t id, struct fp_ring *q_ingress,
			struct fp_ring *q_to_endpoints, struct drop_tail_args *args);
	~DropTailRouter();
	virtual void push(struct emu_packet *packet);
	virtual void pull(uint16_t output, struct emu_packet **packet);
private:
	struct packet_queue output_queue[EMU_ROUTER_NUM_PORTS];
};

/**
 * A drop tail endpoint.
 * @output_queue: a queue of packets to be sent out
 */
class DropTailEndpoint : public Endpoint {
public:
	DropTailEndpoint(uint16_t id, struct drop_tail_args *args);
	~DropTailEndpoint();
	virtual void reset();
	// TODO: make these bulk functions
	virtual void new_packet(struct emu_packet *packet);
	virtual void push(struct emu_packet *packet);
	virtual void pull(struct emu_packet **packet);
private:
	struct packet_queue	output_queue;
};

/**
 * A drop tail endpoint group.
 */
class DropTailEndpointGroup : public EndpointGroup {
public:
	DropTailEndpointGroup(uint16_t num_endpoints,
			struct fp_ring *q_new_packets, struct fp_ring *q_from_network,
			struct fp_ring *q_to_router) : EndpointGroup(num_endpoints,
					q_new_packets, q_from_network, q_to_router) {}
	~DropTailEndpointGroup();
	virtual Endpoint *make_endpoint(uint16_t id, struct drop_tail_args *args) {
		return new DropTailEndpoint(id, args);
	}
	/* TODO: replace new_packets/push/pull functions with more efficient batch
	 * versions */
};

#endif /* DROP_TAIL_H_ */
