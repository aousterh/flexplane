/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "config.h"
#include "api.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "router.h"
#include "admissible_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#define ADMITTED_MEMPOOL_SIZE	10
#define ADMITTED_Q_LOG_SIZE		4
#define PACKET_MEMPOOL_SIZE		(1024 * 128)
#define PACKET_Q_LOG_SIZE		12
#define EMU_NUM_PACKET_QS		(2 + EMU_NUM_ROUTERS)

extern struct emu_state *g_state;

/**
 * Data structure to store the state of the emulation.
 * @endpoint_groups: representations of groups of endpoints
 * @routers: representations of routers
 * @q_new_packets: queue of packets from comm core to endpoint groups
 * @admitted_traffic_mempool: pool of admitted traffic structs
 * @admitted: the current admitted struct
 * @q_admitted_out: queue of admitted structs to comm core
 * @packet_mempool: pool of packet structs
 * @stat: stats
 * @core_stats: per-core stats
 */
struct emu_state {
	EndpointGroup							*endpoint_groups[EMU_NUM_ENDPOINT_GROUPS];
	Router									*routers[EMU_NUM_ROUTERS];
	struct fp_ring							*q_new_packets[EMU_NUM_ENDPOINT_GROUPS];
	struct fp_mempool						*admitted_traffic_mempool;
	struct emu_admitted_traffic				*admitted;
	struct fp_ring							*q_admitted_out;
	struct fp_mempool						*packet_mempool;
	struct emu_admission_statistics			stat;
	struct emu_admission_core_statistics	core_stats; /* 1 core for now */
};

/**
 * Add backlog from src to dst for flow.
 */
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount);

/**
 * Emulate a single timeslot.
 */
void emu_emulate(struct emu_state *state);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void emu_cleanup(struct emu_state *state);

/**
 * Reset the emulation state for a single sender.
 */
void emu_reset_sender(struct emu_state *state, uint16_t src);

/**
 * Initialize an emulation state.
 */
void emu_init_state(struct emu_state *state,
	    struct fp_mempool *admitted_traffic_mempool,
	    struct fp_ring *q_admitted_out,
	    struct fp_mempool *packet_mempool,
	    struct fp_ring **packet_queues, void *args);

/**
 * Returns an initialized emulation state, or NULL on error.
 */
struct emu_state *emu_create_state(struct fp_mempool *admitted_traffic_mempool,
		   struct fp_ring *q_admitted_out,
		   struct fp_mempool *packet_mempool,
		   struct fp_ring **packet_queues, void *args);

#endif /* EMULATION_H_ */
