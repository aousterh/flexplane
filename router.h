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
 * @push_batch: push a batch of packets from q_ingress to output queues
 * @push: enqueue a single packet to this router
 * @pull: dequeue a single packet from port output at this router
 * @id: the unique id of this router
 * @q_ingress: queue of incoming packets
 */
class Router {
public:
	Router(uint16_t id, struct fp_ring *q_ingress);
	~Router();
	virtual void push_batch();
	// TODO: make these bulk functions
	virtual void push(struct emu_packet *packet) = 0;
	virtual void pull(uint16_t output, struct emu_packet **packet) = 0;
private:
	uint16_t id;
	struct fp_ring	*q_ingress;
};

#endif /* ROUTER_H_ */
