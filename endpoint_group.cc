/*
 * endpoint_group.cc
 *
 *  Created on: December 11, 2014
 *      Author: aousterh
 */

#include "endpoint_group.h"
#include "endpoint.h"
#include "packet.h"
#include "assert.h"
#include <stddef.h>

EndpointGroup::~EndpointGroup() {
	uint16_t i;
	struct emu_packet *p;

	for (i = 0; i < num_endpoints; i++)
		delete endpoints[i];
}

void EndpointGroup::init(uint16_t start_id, struct drop_tail_args *args) {
	uint16_t i;

	/* initialize all the endpoints */
	for (i = 0; i < num_endpoints; i++) {
		endpoints[i] = make_endpoint(start_id + i, args);
		assert(endpoints[i] != NULL);
	}
}

void EndpointGroup::reset(uint16_t endpoint_id) {
	endpoints[endpoint_id]->reset();
}

void EndpointGroup::new_packets(struct emu_packet **pkts, uint32_t n_pkts) {
	uint32_t i;
	Endpoint *ep;

	/* pass packets from network to endpoints */
	for (i = 0; i < n_pkts; i++) {
		ep = endpoints[pkts[i]->src];
		ep->new_packet(pkts[i]);
	}
}

void EndpointGroup::push_batch(struct emu_packet **pkts, uint32_t n_pkts) {
	uint16_t i;
	Endpoint *ep;

	for (i = 0; i < n_pkts; i++) {
		ep = endpoints[pkts[i]->dst];
		ep->push(pkts[i]);
	}
}

uint32_t EndpointGroup::pull_batch(struct emu_packet **pkts) {
	uint32_t i, count;
	Endpoint *ep;

	// TODO: do in random order
	/* dequeue one packet from each endpoint, send to next hop in network */
	count = 0;
	for (i = 0; i < num_endpoints; i++) {
		ep = endpoints[i];
		ep->pull(&pkts[count]);

		if (pkts[count] != NULL)
			count++;
	}
	assert(count <= num_endpoints);
	return count;
}
