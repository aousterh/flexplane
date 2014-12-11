/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <inttypes.h>

struct emu_packet;
struct fp_ring;

/**
 * A representation of a router in the emulated network.
 * @emulate: emulate one timeslot at this router
 * @push: enqueue a single packet to this router
 * @pull: dequeue a single packet from port output at this router
 * @id: the unique id of this router
 * @q_ingress: queue of incoming packets
 * @q_to_endpoints: queue of packets to endpoints
 */
class Router {
public:
	Router(uint16_t id, struct fp_ring *q_ingress,
			struct fp_ring *q_to_endpoints);
	~Router();
	virtual void emulate();
	// TODO: make these bulk functions
	virtual void push(struct emu_packet *packet) = 0;
	virtual void pull(uint16_t output, struct emu_packet **packet) = 0;
private:
	uint16_t id;
	struct fp_ring	*q_ingress;
	struct fp_ring	*q_to_endpoints;
};

#endif /* ROUTER_H_ */
