/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "config.h"
#include "endpoint.h"
#include "router.h"
#include "../graph-algo/admissible_algo_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#define ADMITTED_MEMPOOL_SIZE 10
#define ADMITTED_Q_LOG_SIZE 4
#define PACKET_MEMPOOL_SIZE (1024 * 32)
#define PACKET_Q_LOG_SIZE 12

/**
 * Data structure to store the state of the emulation.
 */
struct emu_state {
        struct emu_endpoint endpoints[EMU_NUM_ENDPOINTS];
        struct emu_router routers[EMU_NUM_ROUTERS];
        struct fp_mempool *admitted_traffic_mempool;
        struct fp_ring *q_admitted_out;
        struct fp_mempool *packet_mempool;
        struct admission_statistics stat;
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
struct emu_state *emu_create_state(struct fp_mempool *admitted_traffic_mempool,
                                   struct fp_ring *q_admitted_out,
                                   struct fp_mempool *packet_mempool,
                                   struct fp_ring **packet_queues,
                                   uint16_t router_output_port_capacity);

#endif /* EMULATION_H_ */
