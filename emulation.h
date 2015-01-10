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
#include "endpoint.h"
#include "router.h"
#include "queue_bank_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "../protocol/topology.h"
#include <inttypes.h>

#define ADMITTED_MEMPOOL_SIZE	10
#define ADMITTED_Q_LOG_SIZE		4
#define PACKET_MEMPOOL_SIZE		(1024 * 16)
#define PACKET_Q_LOG_SIZE		12
#define EMU_NUM_PACKET_QS		(2 + EMU_NUM_ROUTERS)

/* comm core state - 1 comm core right now */
#define EPGS_PER_COMM			(EMU_NUM_ENDPOINT_GROUPS)

struct emu_state;

extern struct emu_state *g_state;

#ifdef __cplusplus
class EndpointGroup;
class Router;
class EmulationOutput;
class EndpointDriver;
class RouterDriver;
#endif

/**
 * Emu state allocated for each comm core
 * @q_epg_new_pkts: queues of packets from comm core to each endpoint group
 */
struct emu_comm_state {
	struct fp_ring	*q_epg_new_pkts[EPGS_PER_COMM];
};

/**
 * Data structure to store the state of the emulation.
 * @admitted: the current admitted struct
 * @packet_mempool: pool of packet structs
 * @admitted_traffic_mempool: pool of admitted traffic structs
 * @q_admitted_out: queue of admitted structs to comm core
 * @stat: global emulation stats
 * @core_stats: stats per emulation core
 * @comm_state: state allocated per comm core to manage new packets
 * @queue_bank_stats: stats about one queue bank to be output by the log core
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
	struct emu_comm_state					comm_state;
	struct queue_bank_stats					queue_bank_stats;

	/* this state is not directly accessible from the arbiter */
#ifdef __cplusplus
	EmulationOutput	*out;

	EndpointGroup	*endpoint_groups[EMU_NUM_ENDPOINT_GROUPS];
	struct fp_ring	*q_epg_ingress[EMU_NUM_ENDPOINT_GROUPS];
	EndpointDriver	*endpoint_drivers[EMU_NUM_ENDPOINT_GROUPS];
	Router			*routers[EMU_NUM_ROUTERS];
	struct fp_ring	*q_router_ingress[EMU_NUM_ROUTERS];
	RouterDriver	*router_drivers[EMU_NUM_ROUTERS];
#endif
};

/**
 * Initialize an emulation state.
 */
void emu_init_state(struct emu_state *state,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_ring *q_admitted_out, struct fp_mempool *packet_mempool,
		struct fp_ring **packet_queues, enum RouterType r_type, void *r_args,
		enum EndpointType e_type, void *e_args);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void emu_cleanup(struct emu_state *state);

/**
 * Allocates rings and mempools, and initializes an emulation state
 */
#ifdef NO_DPDK
void emu_alloc_init(struct emu_state *state,
		uint32_t admitted_mempool_size, uint32_t admitted_ring_size,
		uint32_t packet_mempool_size, uint32_t packet_ring_size);
#endif

/**
 * Add backlog from @src to @dst for @flow. Add @amount MTUs, with the first id
 * of @start_id.
 */
static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount, uint16_t start_id);

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
