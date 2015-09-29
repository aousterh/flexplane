/*
 * endpoint_group.cc
 *
 *  Created on: December 11, 2014
 *      Author: aousterh
 */

#include "endpoint_group.h"
#include "endpoint.h"
#include "queue_managers/drop_tail.h"
#include "packet.h"
#include "simple_endpoint.h"
#include "../protocol/platform/generic.h"
#include "../graph-algo/random.h"
#include <assert.h>
#include <stddef.h>


EndpointGroup *EndpointGroupFactory::NewEndpointGroup(enum EndpointType type,
		uint16_t start_id, void *args, struct emu_topo_config *topo_config) {
	struct rate_limiting_ep_args *rl_args;
	uint16_t q_capacity;

	switch(type) {
	case(E_Simple):
		q_capacity = (args == NULL) ? SIMPLE_ENDPOINT_QUEUE_CAPACITY :
				((struct simple_ep_args *) args)->q_capacity;
		return new SimpleEndpointGroup(start_id, q_capacity, topo_config);
	case(E_Rate_limiting):
			assert(args != NULL);
			rl_args = (struct rate_limiting_ep_args *) args;
			return new RateLimitingEndpointGroup(start_id, rl_args->q_capacity,
					rl_args->t_btwn_pkts, topo_config);
	case(E_SimpleTSO):
			q_capacity = (args == NULL) ? SIMPLE_ENDPOINT_QUEUE_CAPACITY :
					((struct simple_ep_args *) args)->q_capacity;
			return new SimpleTSOEndpointGroup(start_id, q_capacity,
					topo_config);
	}
	throw std::runtime_error("invalid endpoint type\n");
}

EndpointGroup::EndpointGroup()
{}

EndpointGroup::~EndpointGroup()
{}
