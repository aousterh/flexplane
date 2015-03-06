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

#define ADMITTED_MEMPOOL_SIZE	128
#define ADMITTED_Q_LOG_SIZE		4
#define PACKET_MEMPOOL_SIZE		(1024 * 1024)
#define PACKET_Q_LOG_SIZE		16
#define EMU_NUM_PACKET_QS		(3 * EMU_NUM_ENDPOINT_GROUPS + EMU_NUM_ROUTERS)

#ifdef __cplusplus
class EmulationCore;
class EndpointGroup;
class Router;
class EmulationOutput;
class EndpointDriver;
class RouterDriver;
#endif

/**
 * Emu state allocated for each comm core
 * @q_epg_new_pkts: queues of packets from comm core to each endpoint group
 * @q_resets: a queue of pending resets, for each endpoint group
 */
struct emu_comm_state {
	struct fp_ring	*q_epg_new_pkts[EPGS_PER_COMM];
	struct fp_ring	*q_resets[EPGS_PER_COMM];
};

/**
 * Data structure to store the state of the emulation.
 * @packet_mempool: pool of packet structs
 * @admitted_traffic_mempool: pool of admitted traffic structs
 * @q_admitted_out: queue of admitted structs to comm core
 * @stat: global emulation stats (mostly used by comm core)
 * @comm_state: state allocated per comm core to manage new packets
 * @queue_bank_stats: pointers to stats for each queue bank
 * @port_drop_stats: pointers to stats for each router
 * @core_stats: per-core stats, for easier access from logging core
 * @cores: the emulation cores
 */
struct emu_state {
	struct fp_mempool						*packet_mempool;
	struct fp_mempool						*admitted_traffic_mempool;
	struct fp_ring							*q_admitted_out;
	struct emu_admission_statistics			stat;
	struct emu_comm_state					comm_state;
	struct queue_bank_stats					*queue_bank_stats[EMU_NUM_ROUTERS];
	struct port_drop_stats					*port_drop_stats[EMU_NUM_ROUTERS];
	struct emu_admission_core_statistics	*core_stats[ALGO_N_CORES];

	/* this state is not directly accessible from the arbiter */
#ifdef __cplusplus
	EmulationCore							*cores[ALGO_N_CORES];
#endif
};

/**
 * Initialize an emulation state.
 */
void emu_init_state(struct emu_state *state,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_ring *q_admitted_out, struct fp_mempool *packet_mempool,
		uint32_t packet_ring_size, enum RouterType r_type, void *r_args,
		enum EndpointType e_type, void *e_args);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void emu_cleanup(struct emu_state *state);

/**
 * Emulate a single timeslot - called only for single-core tests. For
 * multicore, should call step on the specific core instead.
 */
void emu_emulate(struct emu_state *state);

/**
 * Add backlog from @src to @dst for @flow. Add @amount MTUs, with the first id
 * of @start_id. @areq_data provides additional information about each MTU.
 */
static inline
void emu_add_backlog(struct emu_state *state, uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount, uint16_t start_id, u8* areq_data);

/**
 * Reset the emulation state for a single sender @src.
 */
static inline
void emu_reset_sender(struct emu_state *state, uint16_t src);

#endif /* EMULATION_H_ */
