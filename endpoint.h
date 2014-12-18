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

/**
 * A representation of an endpoint (server) in the emulated network.
 * @reset: reset state when an endpoint loses sync with the arbiter
 * @new_packet: enqueue a single packet to this endpoint from the network stack
 * @push: enqueue a single packet to this endpoint from the network
 * @pull: dequeue a single packet from this endpoint to send on the network
 * @id: the unique id of this endpoint
 */
class Endpoint {
public:
	Endpoint(uint16_t id) : id(id) {};
	virtual ~Endpoint() {};
	virtual void reset() {};
	virtual void new_packet(struct emu_packet *packet) {};
	virtual void push(struct emu_packet *packet) {};
	virtual void pull(struct emu_packet **packet) {};
	uint16_t id;
};

#endif /* ENDPOINT_H_ */
