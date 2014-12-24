/*
 * endpoint.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include <inttypes.h>

struct emu_packet;

enum EndpointType {
	E_DropTail
};

#ifdef __cplusplus
/**
 * A representation of an endpoint (server) in the emulated network.
 * @reset: reset state when an endpoint loses sync with the arbiter
 * @id: the unique id of this endpoint
 */
class Endpoint {
public:
	Endpoint(uint16_t id) : id(id) {};
	virtual ~Endpoint() {};
	virtual void reset() {};
	uint16_t id;
};
#endif

#endif /* ENDPOINT_H_ */
