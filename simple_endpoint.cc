/*
 * simple_endpoint.cc
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#include "simple_endpoint.h"
#include "api.h"
#include "api_impl.h"

#define SIMPLE_ENDPOINT_QUEUE_CAPACITY 4096

SimpleEndpoint::SimpleEndpoint(uint16_t id, struct simple_ep_args *args,
		EmulationOutput &emu_output)
	: Endpoint(id), m_emu_output(emu_output)
{
    uint32_t qmax;

    /* use args if supplied, otherwise use defaults */
    if (args != NULL)
        qmax = args->q_capacity;
    else
        qmax = SIMPLE_ENDPOINT_QUEUE_CAPACITY;
    
    queue_create(&output_queue, qmax);
}

SimpleEndpoint::~SimpleEndpoint() {
	reset();
}

void SimpleEndpoint::reset() {
	struct emu_packet *packet;

	/* dequeue all queued packets */
	while (queue_dequeue(&output_queue, &packet) == 0)
		m_emu_output.free_packet(packet);
}

SimpleEndpointGroup::SimpleEndpointGroup(uint16_t num_endpoints,
		EmulationOutput &emu_output, uint16_t start_id,
		struct simple_ep_args *args)
	: EndpointGroup(num_endpoints, emu_output)  {
	CONSTRUCT_ENDPOINTS(SimpleEndpoint, start_id, args, num_endpoints,
		endpoints, emu_output);
}

SimpleEndpointGroup::~SimpleEndpointGroup() {
	DESTRUCT_ENDPOINTS(num_endpoints, endpoints);
}

void SimpleEndpointGroup::reset(uint16_t endpoint_id) {
	RESET_ENDPOINT(endpoints, endpoint_id);
}

void SimpleEndpointGroup::new_packets(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_NEW_PACKETS(SimpleEndpoint, new_packet, pkts, n_pkts,
			endpoints);
}

void SimpleEndpointGroup::push_batch(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_PUSH_BATCH(SimpleEndpoint, push, pkts, n_pkts, endpoints);
}

uint32_t SimpleEndpointGroup::pull_batch(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_PULL_BATCH(SimpleEndpoint, pull, pkts, n_pkts, random_state,
			num_endpoints, endpoints);
}
