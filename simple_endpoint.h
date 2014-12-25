/*
 * simple_endpoint.h
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#ifndef SIMPLE_ENDPOINT_H_
#define SIMPLE_ENDPOINT_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/random.h"
#include "output.h"

struct packet_queue;

struct simple_ep_args {
    uint16_t q_capacity;
};

/**
 * A drop tail endpoint.
 * @output_queue: a queue of packets to be sent out
 */
class SimpleEndpoint : public Endpoint {
public:
	SimpleEndpoint(uint16_t id, struct simple_ep_args *args,
			EmulationOutput &emu_output);
	~SimpleEndpoint();
	virtual void reset();
	void inline new_packet(struct emu_packet *packet) {
		/* try to enqueue the packet */
		if (queue_enqueue(&output_queue, packet) != 0) {
			/* no space to enqueue, drop this packet */
			adm_log_emu_endpoint_dropped_packet(&g_state->stat);
			m_emu_output.drop(packet);
		}
	}
	void inline push(struct emu_packet *packet) {
		assert(packet->dst == id);

		/* pass the packet up the stack */
		m_emu_output.admit(packet);
	}
	void inline pull(struct emu_packet **packet) {
		/* dequeue one incoming packet if the queue is non-empty */
		*packet = NULL;
		queue_dequeue(&output_queue, packet);
	}
private:
	struct packet_queue	output_queue;
	EmulationOutput &m_emu_output;
};

/**
 * A drop tail endpoint group.
 */
class SimpleEndpointGroup : public EndpointGroup {
public:
	SimpleEndpointGroup(uint16_t num_endpoints, EmulationOutput &emu_output,
			uint16_t start_id, struct simple_ep_args *args);
	~SimpleEndpointGroup();
	virtual void reset(uint16_t endpoint_id);
	virtual void new_packets(struct emu_packet **pkts, uint32_t n_pkts);
	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts);
private:
	SimpleEndpoint	*endpoints[MAX_ENDPOINTS_PER_GROUP];
};

#endif /* SIMPLE_ENDPOINT_H_ */
