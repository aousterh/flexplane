/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "admissible_log.h"
#include "config.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include <inttypes.h>

#define ADMITTED_MEMPOOL_SIZE	10
#define ADMITTED_Q_LOG_SIZE		4
#define PACKET_MEMPOOL_SIZE		(1024 * 128)
#define PACKET_Q_LOG_SIZE		12
#define EMU_NUM_PACKET_QS		(2 + EMU_NUM_ROUTERS)

struct emu_state;

extern struct emu_state *g_state;

#ifdef __cplusplus
class EndpointGroup;
class Router;
#endif

/**
 * Data structure to store the state of the emulation.
 * @admitted: the current admitted struct
 * @packet_mempool: pool of packet structs
 * @admitted_traffic_mempool: pool of admitted traffic structs
 * @q_admitted_out: queue of admitted structs to comm core
 * @stat: global emulation stats
 * @core_stats: stats per emulation core
 * @q_epg_new_pkts: queues of packets from comm core to each endpoint group
 * @endpoint_groups: representations of groups of endpoints
 * @q_epg_ingress: queues of packets to endpoint groups from the network
 * @routers: representations of routers
 * @q_router_ingress: a queue of incoming packets for each router
 */
struct emu_state {
	struct emu_admitted_traffic				*admitted;
	struct fp_mempool						*packet_mempool;
	struct fp_mempool						*admitted_traffic_mempool;
	struct fp_ring							*q_admitted_out;
	struct emu_admission_statistics			stat;
//	struct emu_admission_core_statistics	core_stats; /* 1 core for now */
	struct fp_ring							*q_epg_new_pkts[EMU_NUM_ENDPOINT_GROUPS];

	/* this state is not directly accessible from the arbiter */
#ifdef __cplusplus
	EndpointGroup	*endpoint_groups[EMU_NUM_ENDPOINT_GROUPS];
	struct fp_ring	*q_epg_ingress[EMU_NUM_ENDPOINT_GROUPS];
	Router			*routers[EMU_NUM_ROUTERS];
	struct fp_ring	*q_router_ingress[EMU_NUM_ROUTERS];
#endif
};

/**
 * Initialize an emulation state.
 */
void emu_init_state(struct emu_state *state,
	    struct fp_mempool *admitted_traffic_mempool,
	    struct fp_ring *q_admitted_out,
	    struct fp_mempool *packet_mempool,
	    struct fp_ring **packet_queues, void *args);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void emu_cleanup(struct emu_state *state);

/**
 * Add backlog from src to dst for flow.
 */
static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount);

/**
 * Emulate a single timeslot.
 */
void emu_emulate(struct emu_state *state);

/**
 * Reset the emulation state for a single sender.
 */
#ifdef __cplusplus
extern "C" void emu_reset_sender(struct emu_state *state, uint16_t src);
#else
void emu_reset_sender(struct emu_state *state, uint16_t src);
#endif

#endif /* EMULATION_H_ */
