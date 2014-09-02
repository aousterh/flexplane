/*
 * port.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef PORT_H_
#define PORT_H_

#include <assert.h>
#include <stdlib.h>

struct emu_endpoint;
struct emu_router;

/* NICs can be at either at endpoint or a router */
enum nic_type {
	NIC_TYPE_ENDPOINT,
	NIC_TYPE_ROUTER,
};

/**
 * A port in the emulated network.
 * @q: a queue of packets traversing this port
 * @ingress_type: the type of ingress (endpoint or router)
 * @ingress_ep: a pointer to the ingress enpoint
 * @ingress_rtr: a pointer to the ingress router
 * @egress_type: the type of egress (endpoint or router)
 * @egress_ep: a pointer to the egress endpoint
 * @egress_rtr: a pointer to the egress router
 */
struct emu_port {
	struct fp_ring			*q;
	enum nic_type			ingress_type;
	union {
		struct emu_endpoint *ingress_ep;
		struct emu_router *ingress_rtr;
	};
	enum nic_type			egress_type;
	union {
		struct emu_endpoint *egress_ep;
		struct emu_router *egress_rtr;
	};
};

/**
 * Initialize a port.
 * @return 0 on success, negative value on error.
 */
static inline
int port_init(struct emu_port *port, struct fp_ring *q,
		enum nic_type ingress_type, void *ingress,
		enum nic_type egress_type, void *egress) {
	assert(port != NULL);
	assert(q != NULL);
	assert(ingress != NULL);
	assert(egress != NULL);

	port->q = q;

	port->ingress_type = ingress_type;
	if (port->ingress_type == NIC_TYPE_ENDPOINT)
		port->ingress_ep = (struct emu_endpoint *) ingress;
	else
		port->ingress_rtr = (struct emu_router *) ingress;

	port->egress_type = egress_type;
	if (port->egress_type == NIC_TYPE_ENDPOINT)
		port->egress_ep = (struct emu_endpoint *) egress;
	else
		port->egress_rtr = (struct emu_router *) egress;

	return 0;
}

#endif /* PORT_H_ */
