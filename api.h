/*
 * api.h
 *
 *  Created on: Aug 28, 2014
 *      Author: yonch
 */

#ifndef API_H_
#define API_H_


struct emu_port *router_port(struct emu_router *rtr, uint32_t port_ind);
struct emu_port *endpoint_port(struct emu_endpoint *ep);

void send_packet(struct emu_port *port, struct emu_packet *packet);

void drop_packet(struct emu_port *port, struct emu_packet *packet);

uint32_t router_id(struct emu_router *rtr);
uint32_t endpoint_id(struct emu_endpoint *ep);


struct router_ops {
	/**
	 * Number of bytes to reserve in a router struct for implementation-
	 * specific data. This private data can be accessed using router_priv().
	 */
	uint32_t router_priv_size;

	/**
	 * Initialize a router
	 * @return 0 on success, negative value on error
	 */
	int (*init)(struct emu_router *rtr);

	/**
	 * Cleanup state and memory. Called when emulation terminates.
	 */
	void (*cleanup)(struct emu_router *rtr);

	/**
	 * Emulate one timeslot at a given router. For now, assume that routers
	 * can process MTUs with no additional delay beyond the queueing delay.
	 */
	void (*emulate)(struct emu_router *router, struct emu_state *state);
};

struct endpoint_ops {
	/**
	 *
	 * Number of bytes to reserve in an endpoint struct for implementation-
	 * specific data. This private data can be accessed using endpoint_priv().
	 */
	uint32_t endpoint_priv_size;

	/**
	 * Initialize an endpoint.
	 * @return 0 on success, negative value on error
	 */
	int (*init)(struct emu_endpoint *ep);

	/**
	 * Reset an endpoint. This happens when endpoints lose sync with the
	 * 	arbiter. To resync, a reset occurs, then backlogs are re-added based
	 * 	on endpoint reports.
	 */
	void (*reset)(struct emu_endpoint *ep);

	/**
	 * Cleanup state and memory. Called when emulation terminates.
	 */
	void (*cleanup)(struct emu_endpoint *ep);

	/**
	 * Add backlog to dst at this endpoint.
	 */
	void (*add_backlog)(struct emu_endpoint *endpoint,
			  	  	  	struct emu_state *state, uint16_t dst,
			  	  	  	uint32_t amount, uint16_t start_id);

	/**
	 * Emulate one timeslot at a given endpoint.
	 */
	void (*emulate)(struct emu_endpoint *endpoint, struct emu_state *state);
};



#endif /* API_H_ */
