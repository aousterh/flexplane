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

#include <inttypes.h>

struct emu_endpoint;
struct emu_packet;
class Router;
struct emu_admission_statistics;

/* alignment macros based on /net/pkt_sched.h */
#define EMU_ALIGNTO				64
#define EMU_ALIGN(len)			(((len) + EMU_ALIGNTO-1) & ~(EMU_ALIGNTO-1))

/*
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

/**
 * Notifies physical endpoint to drop packet and frees packet memory.
 */
static inline
void drop_packet(struct emu_packet *packet);

/**
 * Enqueues a packet at an endpoint to pass up the network stack.
 */
static inline
void enqueue_packet_at_endpoint(struct emu_endpoint *ep,
		struct emu_packet *packet);

/**
 * Frees a packet when an emulation algorithm is done running.
 */
static inline
void free_packet(struct emu_packet *packet);

/**
 * Returns the id of endpoint ep.
 */
static inline
uint32_t endpoint_id(struct emu_endpoint *ep);

/**
 * Creates a packet, returns a pointer to the packet.
 */
struct emu_packet *create_packet(uint16_t src, uint16_t dst, uint16_t flow);

/**
 * Return the queue that packet p should be sent out of at router rtr
 * (static routing).
 */
static inline
uint16_t get_output_queue(Router *rtr, struct emu_packet *p);


/*
 * Logging functions that emulation algorithms may call.
 */

/**
 * Logs that an endpoint sent a packet into the network.
 */
static inline __attribute__((always_inline))
void adm_log_emu_endpoint_sent_packet(struct emu_admission_statistics *st);

/**
 * Logs that a router sent a packet into the network.
 */
static inline __attribute__((always_inline))
void adm_log_emu_router_sent_packet(struct emu_admission_statistics *st);

/**
 * Logs that an endpoint dropped a packet.
 */
static inline __attribute__((always_inline))
void adm_log_emu_endpoint_dropped_packet(struct emu_admission_statistics *st);

/**
 * Logs that a router dropped a packet.
 */
static inline __attribute__((always_inline))
void adm_log_emu_router_dropped_packet(struct emu_admission_statistics *st);


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
	 * Receive a packet @p from the application at endpoint @ep.
	 */
	void (*rcv_from_app)(struct emu_endpoint *ep, struct emu_packet *p);

	/**
	 * Receive a packet @p from the network at endpoint @ep.
	 */
	void (*rcv_from_net)(struct emu_endpoint *ep, struct emu_packet *p);

	/**
	 * Choose a packet @p to send on the network at endpoint @ep.
	 */
	void (*send_to_net)(struct emu_endpoint *ep, struct emu_packet **p);
};

/*
 * One struct to hold all functions and parameters for an emulation algorithm.
 */
struct emu_ops {
	struct endpoint_ops	ep_ops;
	uint32_t			packet_priv_size;
	void				*args; /* struct for any algo-specific args */
};


/*
 * Functions to enable algorithms to access private portions of emulation
 * data structures.
 */

/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *endpoint_priv(struct emu_endpoint *ep);

/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *packet_priv(struct emu_packet *packet);


#endif /* API_H_ */
