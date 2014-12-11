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


/**
 * Returns the private part of the endpoint struct.
 */
static inline
void *packet_priv(struct emu_packet *packet);


#endif /* API_H_ */
