/*
 * api.h
 *
 * The api between the emulation framework and an emulation algorithm.
 *
 *  Created on: Aug 28, 2014
 *      Author: yonch
 */

#ifndef API_H_
#define API_H_

#include "router.h"
#include "endpoint.h"
#include "packet.h"

/* alignment macros based on /net/pkt_sched.h */
#define EMU_ALIGNTO				64
#define EMU_ALIGN(len)			(((len) + EMU_ALIGNTO-1) & ~(EMU_ALIGNTO-1))

/*
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

/**
 * Returns a pointer to the port with index port_ind at router rtr.
 */
struct emu_port *router_port(struct emu_router *rtr, uint32_t port_ind);

/**
 * Returns a pointer to the port at endpoint ep.
 */
struct emu_port *endpoint_port(struct emu_endpoint *ep);

/**
 * Sends packet out port.
 */
void send_packet(struct emu_port *port, struct emu_packet *packet);

/**
 * Drops packet.
 */
void drop_packet(struct emu_packet *packet);

/**
 * Receives a packet from a port. Returns a pointer to the packet, or NULL if
 * none are available at this port.
 */
struct emu_packet *receive_packet(struct emu_port *port);

/**
 * Dequeues a packet at an endpoint from the network stack. Returns a pointer
 * to the packet, or NULL if none are available.
 */
struct emu_packet *dequeue_packet_at_endpoint(struct emu_endpoint *ep);

/**
 * Frees a packet when an emulation algorithm is done running.
 */
void free_packet(struct emu_packet *packet);

/**
 * Returns the id of router rtr.
 */
uint32_t router_id(struct emu_router *rtr);

/**
 * Returns the id of endpoint ep.
 */
uint32_t endpoint_id(struct emu_endpoint *ep);

/**
 * Creates a packet, returns a pointer to the packet.
 */
struct emu_packet *create_packet(uint16_t src, uint16_t dst);


/* Router functions that an emulation algorithm must implement. */

struct router_ops {
	/**
	 * Number of bytes to reserve in a router struct for implementation-
	 * specific data. This private data can be accessed using router_priv().
	 */
	uint32_t priv_size;

	/**
	 * Initialize a router.
	 * @return 0 on success, negative value on error
	 */
	int (*init)(struct emu_router *rtr, void * args);

	/**
	 * Cleanup state and memory. Called when emulation terminates.
	 */
	void (*cleanup)(struct emu_router *rtr);

	/**
	 * Emulate one timeslot at a given router. For now, assume that routers
	 * can process MTUs with no additional delay beyond the queueing delay.
	 */
	void (*emulate)(struct emu_router *rtr);
};


/* Endpoint functions that an emulation algorithm must implement. */

struct endpoint_ops {
	/**
	 * Number of bytes to reserve in an endpoint struct for implementation-
	 * specific data. This private data can be accessed using endpoint_priv().
	 */
	uint32_t priv_size;

	/**
	 * Initialize an endpoint.
	 * @return 0 on success, negative value on error
	 */
	int (*init)(struct emu_endpoint *ep, void *args);

	/**
	 * Reset an endpoint. This happens when endpoints lose sync with the
	 * arbiter. To resync, a reset occurs, then backlogs are re-added based
	 * on endpoint reports.
	 */
	void (*reset)(struct emu_endpoint *ep);

	/**
	 * Cleanup state and memory. Called when emulation terminates.
	 */
	void (*cleanup)(struct emu_endpoint *ep);

	/**
	 * Emulate one timeslot at a given endpoint.
	 */
	void (*emulate)(struct emu_endpoint *ep);
};

/*
 * One struct to hold all functions and parameters for an emulation algorithm.
 */
struct emu_ops {
	struct router_ops	rtr_ops;
	struct endpoint_ops	ep_ops;
	uint32_t			packet_priv_size;
	void				*args; /* struct for any algo-specific args */
};


/*
 * Functions to enable algorithms to access private portions of emulation
 * data structures.
 */

/**
 * Returns the private part of the router struct.
 */
static inline
void *router_priv(struct emu_router *rtr) {
	return (char *) rtr + EMU_ALIGN(sizeof(struct emu_router));
}

/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *endpoint_priv(struct emu_endpoint *ep) {
	return (char *) ep + EMU_ALIGN(sizeof(struct emu_endpoint));
}

/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *packet_priv(struct emu_packet *packet) {
	return (char *) packet + EMU_ALIGN(sizeof(struct emu_packet));
}


#endif /* API_H_ */
