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
#include "../graph-algo/random.h"
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
	virtual ~EndpointGroup() {};
	virtual void reset(uint16_t id) = 0;
	virtual void new_packets(struct emu_packet **pkts, uint32_t n_pkts) = 0;
	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts) = 0;
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts) = 0;
	uint16_t		num_endpoints;
	uint32_t		random_state;
	EmulationOutput	&m_emu_output;
};

/**
 * Call the constructor for each endpoint in @ep_array. Assigns ids @start_id
 * 	through @start_id + @num_eps to the endpoints.
 */
#define CONSTRUCT_ENDPOINTS(endpoint_type, start_id, args, num_eps, ep_array,	\
	emu_output)																	\
{																				\
	uint16_t i;																	\
	/* initialize all the endpoints */											\
	for (i = 0; i < num_eps; i++) {												\
		ep_array[i] = new endpoint_type(start_id + i, args, emu_output);		\
		assert(ep_array[i] != NULL);											\
	}																			\
}

/**
 * Call the destructor for each endpoint in @ep_array.
 */
#define DESTRUCT_ENDPOINTS(num_eps, ep_array)	\
{												\
	uint16_t i;									\
	for (i = 0; i < num_eps; i++)				\
		delete ep_array[i];						\
}

/**
 * Call reset on the endpoint in @ep_array with id @ep_id.
 */
#define RESET_ENDPOINT(ep_array, ep_id)	{ ep_array[ep_id]->reset(); }

/**
 * Call @func to process a new packet for each packet in @pkts at an
 * 	endpoint of type @type.
 */
#define ENDPOINTS_NEW_PACKETS(type, func, pkts, n_pkts, ep_array)	\
{																	\
	uint32_t i;														\
	type *ep;														\
																	\
	/* pass packets from network to endpoints */					\
	for (i = 0; i < n_pkts; i++) {									\
		ep = ep_array[pkts[i]->src];								\
		ep->func(pkts[i]);											\
	}																\
}

/**
 * Call @func for each packet in @pkts at an endpoint of type @type.
 */
#define ENDPOINTS_PUSH_BATCH(type, func, pkts, n_pkts, ep_array)	\
{																	\
	uint16_t i;														\
	type *ep;														\
																	\
	for (i = 0; i < n_pkts; i++) {									\
		ep = ep_array[pkts[i]->dst];								\
		ep->push(pkts[i]);											\
	}																\
}

/**
 * Attempt to pull a packet from each endpoint in @ep_array. Put them in @pkts.
 */
#define ENDPOINTS_PULL_BATCH(type, func, pkts, n_pkts, random_state, num_eps,	\
	ep_array)																	\
{																				\
	uint32_t i, j, count;														\
	type *ep;																	\
	uint32_t endpoint_order[MAX_ENDPOINTS_PER_GROUP];							\
	assert(n_pkts >= num_eps);													\
																				\
	/* generate a random permutation of endpoint indices. use the				\
	 * Fisher-Yates/Knuth shuffle. */											\
	for (i = 0; i < num_endpoints; i++) {										\
		/* choose random index <= i to swap with */								\
		j = random_int(&random_state, i + 1);									\
																				\
		/* swap the element at index j with i */								\
		endpoint_order[i] = endpoint_order[j];									\
		endpoint_order[j] = i;													\
	}																			\
																				\
	/* dequeue one packet from each endpoint, send to next hop in network.		\
	 * process endpoints in a random order */									\
	 count = 0;																	\
	for (i = 0; i < num_eps; i++) {												\
		ep = ep_array[endpoint_order[i]];										\
		ep->func(&pkts[count]);													\
																				\
		if (pkts[count] != NULL)												\
			count++;															\
	}																			\
	assert(count <= num_eps);													\
	return count;																\
}

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
