/*
 * endpoint_group.h
 *
 *  Created on: December 11, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_GROUP_H_
#define ENDPOINT_GROUP_H_

#include "config.h"
#include "endpoint.h"

#define MAX_ENDPOINTS_PER_GROUP		EMU_NUM_ENDPOINTS

/**
 * A representation of a group of endpoints (servers) with a shared router
 * @reset: reset state of an endpoint (e.g. when it loses sync with the arbiter)
 * @push: enqueue packets to these endpoints from the network
 * @pull: dequeue packets from these endpoints to send on the network
 * @make_endpoint: create an endpoint of this class
 * @new_packets: enqueue packets to these endpoints from the network stack
 * @num_endpoints: the number of endpoints in this group
 * @endpoints: a pointer to each endpoint in this group
 * @q_new_packets: queue of incoming packets from the network stack
 * @q_from_network: queue of incoming packets from the network
 * @q_to_router: queue of outgoing packets to the router
 */
class EndpointGroup {
public:
	EndpointGroup(uint16_t num_endpoints, struct fp_ring *q_new_packets,
			struct fp_ring *q_from_network, struct fp_ring *q_to_router);
	~EndpointGroup();
	virtual void init(uint16_t start_id, struct drop_tail_args *args);
	virtual void reset(uint16_t id);
	// TODO: make these bulk functions
	virtual void new_packets();
	virtual void push();
	virtual void pull();
	virtual Endpoint *make_endpoint(uint16_t id,
			struct drop_tail_args *args) = 0;
	void enqueue_packet_from_network(struct emu_packet *p);
private:
	uint16_t		num_endpoints;
	Endpoint		*endpoints[MAX_ENDPOINTS_PER_GROUP];
	struct fp_ring	*q_new_packets;
	struct fp_ring	*q_from_network;
	struct fp_ring	*q_to_router;
};

#endif /* ENDPOINT_GROUP_H_ */
