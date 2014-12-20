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
#include <assert.h>
#include <time.h> /* for seeding the random number generator */
#include "output.h"

#define MAX_ENDPOINTS_PER_GROUP		EMU_NUM_ENDPOINTS

/**
 * A representation of a group of endpoints (servers) with a shared router
 * @init: initialize an endpoint group, starting with @start_id
 * @reset: reset state of an endpoint (e.g. when it loses sync with the arbiter)
 * @new_packets: enqueue packets from the network stack to these endpoints
 * @push_batch: enqueue packets to these endpoints from the network
 * @pull_batch: dequeue packets from these endpoints to send on the network
 * @make_endpoint: create an endpoint of this class
 * @num_endpoints: the number of endpoints in this group
 * @endpoints: a pointer to each endpoint in this group
 * @random_state: int used for generation of random numbers
 */
class EndpointGroup {
public:
	EndpointGroup(uint16_t num_endpoints, EmulationOutput &emu_output);
	virtual ~EndpointGroup();
	virtual void reset(uint16_t id);
	virtual void new_packets(struct emu_packet **pkts, uint32_t n_pkts);
	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts);
	Endpoint		*endpoints[MAX_ENDPOINTS_PER_GROUP];
private:
	uint16_t		num_endpoints;
	uint32_t		random_state;
	EmulationOutput	&m_emu_output;
};

/**
 * Calls the constructors for each endpoint in this endpoint group.
 */
#define CONSTRUCT_ENDPOINTS(start_id, num_endpoints, endpoint_type, args)	\
	{																		\
		uint16_t i;															\
		/* initialize all the endpoints */									\
		for (i = 0; i < num_endpoints; i++) {								\
			endpoints[i] = new endpoint_type(start_id + i, args);			\
			assert(endpoints[i] != NULL);									\
		}																	\
	}																		\

/**
 * A class for constructing endpoint groups of different types.
 * @NewEndpointGroup: constructs an endpoint group
 */
class EndpointGroupFactory {
public:
	static EndpointGroup *NewEndpointGroup(enum EndpointType type,
			uint16_t num_endpoints, EmulationOutput &emu_output,
			uint16_t start_id, void *args);
};

#endif /* ENDPOINT_GROUP_H_ */
