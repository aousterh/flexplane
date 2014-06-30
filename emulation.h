/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "endpoint.h"
#include "switch.h"
#include "topology.h"

#define PACKET_MEMPOOL_SIZE 10000
#define PACKET_Q_SIZE 10

/**
 * Data structure to store the state of the emulation.
 */
struct emu_state {
        struct emu_endpoint endpoints[EMU_NUM_ENDPOINTS];
        struct emu_switch tors[EMU_NUM_TORS];
        struct fp_mempool *packet_mempool;
        struct fp_ring *finished_packet_q;
};

/**
 * Add backlog from src to dst.
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
                     uint32_t amount, uint16_t start_id);

/**
 * Emulate a single timeslot.
 */
void emu_timeslot(struct emu_state *state);

/**
 * Reset the emulation state (clear all demands, packets, etc.).
 */
void emu_reset_state(struct emu_state *state);

/**
 * Returns an initialized emulation state, or NULL on error.
 */
struct emu_state *emu_create_state(struct fp_mempool *packet_mempool,
                                   struct fp_ring **packet_queues);

#endif /* EMULATION_H_ */
