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

struct emu_admission_statistics;
struct emu_packet;
struct emu_state;

/* alignment macros based on /net/pkt_sched.h */
#define EMU_ALIGNTO				64
#define EMU_ALIGN(len)			(((len) + EMU_ALIGNTO-1) & ~(EMU_ALIGNTO-1))

/*
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

/**
 * Creates a packet, returns a pointer to the packet.
 */
static inline
struct emu_packet *create_packet(struct emu_state *state, uint16_t src,
		uint16_t dst, uint16_t flow, uint16_t id);

/**
 * Frees a packet when an emulation algorithm is done running.
 */
static inline
void free_packet(struct emu_state *state, struct emu_packet *packet);

/*
 * Logging functions that emulation algorithms may call.
 */

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
