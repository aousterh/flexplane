/*
 * endpoint_group.cc
 *
 *  Created on: December 11, 2014
 *      Author: aousterh
 */

#include "endpoint_group.h"
#include "endpoint.h"
#include "drop_tail.h"
#include "packet.h"
#include "../protocol/platform/generic.h"
#include "../graph-algo/random.h"
#include <assert.h>
#include <stddef.h>

EndpointGroup::EndpointGroup(uint16_t num_endpoints, EmulationOutput &emu_output)
	: num_endpoints(num_endpoints), m_emu_output(emu_output)
{
	seed_random(&random_state, time(NULL));
}

EndpointGroup *EndpointGroupFactory::NewEndpointGroup(enum EndpointType type,
		uint16_t num_endpoints, EmulationOutput &emu_output, uint16_t start_id,
		void *args) {
	switch(type) {
	case(E_DropTail):
		return new DropTailEndpointGroup(num_endpoints, emu_output, start_id,
				(struct drop_tail_args *) args);
	}
	throw std::runtime_error("invalid endpoint type\n");
}
