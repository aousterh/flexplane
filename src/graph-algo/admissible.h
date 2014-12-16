/*
 * admissible.h
 *
 *  Created on: May 13, 2014
 *      Author: aousterh
 */

#ifndef ADMISSIBLE_H_
#define ADMISSIBLE_H_

#include "algo_config.h"

#include <stdbool.h>

/* dummy struct declarations */
struct admissible_state;
struct admitted_traffic;

/* parallel algo, e.g. pim */
#ifdef PARALLEL_ALGO
#include "admitted.h"
#include "../grant-accept/partitioning.h"
#include "../grant-accept/pim.h"
#include "../grant-accept/pim_admissible_traffic.h"

#define NUM_BINS_SHIFT				0
#define NUM_BINS					1
#define LARGE_BIN_SIZE				0 /* unused */
#define BATCH_SIZE					1
#define BATCH_SHIFT					0
#define ADMITTED_PER_BATCH			N_PARTITIONS
#define NUM_BIN_RINGS				N_PARTITIONS
#define BIN_RING_SHIFT				16
#define MAX_ADMITTED_PER_TIMESLOT	NUM_NODES

static inline
void add_backlog(struct admissible_state *state, uint16_t src, uint16_t dst,
		uint32_t amount) {
	pim_add_backlog((struct pim_state *) state, src, dst, amount);
}

static inline
void flush_backlog(struct admissible_state *state) {
	pim_flush_backlog((struct pim_state *) state);
};

static inline
void get_admissible_traffic(struct admissible_state *state, uint32_t a,
		uint64_t b, uint32_t c, uint32_t d) {
	pim_get_admissible_traffic((struct pim_state *) state);
}

static inline
struct admissible_state *
create_admissible_state(bool a, uint16_t b, uint16_t c, uint16_t d,
		struct fp_ring *e, struct fp_ring *q_admitted_out,
		struct fp_ring *q_spent, struct fp_mempool *bin_mempool,
		struct fp_mempool *admitted_traffic_mempool, struct fp_ring **f,
		struct fp_ring **q_new_demands, struct fp_ring **q_ready_partitions,
		void *g)
{
	(void) q_spent; /* unused */
    struct pim_state *state = pim_create_state(q_new_demands, q_admitted_out,
    		bin_mempool, admitted_traffic_mempool, q_ready_partitions);
    return (struct admissible_state *) state;
}

static inline
void reset_admissible_state(struct admissible_state *state, bool a, uint16_t b,
		uint16_t c, uint16_t d)
{
	pim_reset_state((struct pim_state *) state);
}

static inline
void reset_sender(struct admissible_state *status, uint16_t src)
{
	pim_reset_sender((struct pim_state *) status, src);
}

static inline
void handle_spent_demands(struct admissible_state *state)
{
	/* unused */
}

#endif

/* pipelined algo */
#ifdef PIPELINED_ALGO
#include "admissible_structures.h"
#include "admissible_traffic.h"
#include "admitted.h"
#include "batch.h"

#define ADMITTED_PER_BATCH			BATCH_SIZE
#define NUM_BIN_RINGS				0
#define BIN_RING_SHIFT				0
#define MAX_ADMITTED_PER_TIMESLOT	NUM_NODES

static inline
void add_backlog(struct admissible_state *status, uint16_t src, uint16_t dst,
		uint32_t amount) {
	seq_add_backlog((struct seq_admissible_status *) status, src, dst, amount);
}

static inline
void flush_backlog(struct admissible_state *status) {
	seq_flush_backlog((struct seq_admissible_status *) status);
}

static inline
void get_admissible_traffic(struct admissible_state *status,
		uint32_t core_index, uint64_t first_timeslot, uint32_t tslot_mul,
		uint32_t tslot_shift) {
	seq_get_admissible_traffic((struct seq_admissible_status *) status,
			core_index, first_timeslot, tslot_mul, tslot_shift);
}

static inline
struct admissible_state *
create_admissible_state(bool oversubscribed, uint16_t inter_rack_capacity,
		uint16_t out_of_boundary_capacity, uint16_t num_nodes,
		struct fp_ring *q_head, struct fp_ring *q_admitted_out,
		struct fp_ring *q_spent, struct fp_mempool *head_bin_mempool,
		struct fp_mempool *admitted_traffic_mempool, struct fp_ring **q_bin,
		struct fp_ring **a, struct fp_ring **b, void *c)
{
	struct seq_admissible_status *status;
    status = seq_create_admissible_status(oversubscribed, inter_rack_capacity,
    		out_of_boundary_capacity, num_nodes, q_head, q_admitted_out,
    		q_spent, head_bin_mempool, admitted_traffic_mempool, q_bin);
    return (struct admissible_state *) status;
}

static inline
void reset_admissible_state(struct admissible_state *status,
		bool oversubscribed, uint16_t inter_rack_capacity,
		uint16_t out_of_boundary_capacity, uint16_t num_nodes)
{
	seq_reset_admissible_status((struct seq_admissible_status *) status, oversubscribed,
			inter_rack_capacity, out_of_boundary_capacity, num_nodes);
}

static inline
void reset_sender(struct admissible_state *status, uint16_t src)
{
	seq_reset_sender((struct seq_admissible_status *) status, src);
}

static inline
void handle_spent_demands(struct admissible_state *state)
{
    struct seq_admissible_status *status = (struct seq_admissible_status *) state;
    seq_handle_spent(status);
}

#endif

/* emulation algo */
#ifdef EMULATION_ALGO

#include "../emulation/admitted.h"
#include "../emulation/api.h"
#include "../emulation/emulation.h"
#include "../emulation/emulation_impl.h"
#include "../emulation/packet.h"

#define SMALL_BIN_SIZE				0
#define BATCH_SIZE					1
#define BATCH_SHIFT					0
#define ADMITTED_PER_BATCH			1
#define NUM_BIN_RINGS				EMU_NUM_PACKET_QS
#define BIN_RING_SHIFT				PACKET_Q_LOG_SIZE
#define MAX_ADMITTED_PER_TIMESLOT	(EMU_NUM_ENDPOINTS + EMU_MAX_DROPS)

static inline
void add_backlog(struct admissible_state *state, uint16_t src, uint16_t dst,
		uint32_t amount) {
	uint16_t dst_node;
	uint8_t flow;
	struct emu_state *emu_state = (struct emu_state *) state;

	flow = dst & FLOW_MASK;
	dst_node = dst >> FLOW_SHIFT;
	emu_add_backlog(emu_state, src, dst_node, flow, amount);
}

static inline
void reset_sender(struct admissible_state *state, uint16_t src)
{
	struct emu_state *emu_state = (struct emu_state *) state;
	emu_reset_sender(emu_state, src);
}

static inline
void flush_backlog(struct admissible_state *state) {
	/* unused */
};

static inline
void handle_spent_demands(struct admissible_state *state)
{
	/* unused */
}

static inline
uint16_t get_admitted_partition(struct admitted_traffic *admitted)
{
	return 0; /* emulation does not use partitions */
}

/* get the number of edges admitted */
static inline
uint16_t get_num_admitted(struct admitted_traffic *admitted)
{
	struct emu_admitted_traffic *emu_admitted;
	emu_admitted = (struct emu_admitted_traffic *) admitted;
	return emu_admitted->admitted;
}

/* get the total size (edges admitted and dropped) */
static inline
uint16_t get_size(struct admitted_traffic *admitted)
{
	struct emu_admitted_traffic *emu_admitted;
	emu_admitted = (struct emu_admitted_traffic *) admitted;
	return emu_admitted->size;
}

static inline
struct emu_admitted_edge *get_admitted_edge(struct admitted_traffic *admitted,
					    uint16_t i)
{
	struct emu_admitted_traffic *emu_admitted;
	emu_admitted = (struct emu_admitted_traffic *) admitted;
	return &emu_admitted->edges[i];
}

static inline
uint32_t bin_num_bytes(uint32_t param)
{
	/* bin mempool used for packets instead */
    return sizeof(struct emu_packet);
}

/* functions called only in benchmark_graph_algo (not arbiter) */

static inline
void get_admissible_traffic(struct admissible_state *state, uint32_t a,
		uint64_t b, uint32_t c, uint32_t d) {
	struct emu_state *emu_state = (struct emu_state *) state;
	emu_emulate(emu_state);
}

static inline
struct admissible_state *
create_admissible_state(bool a, uint16_t b, uint16_t c, uint16_t d,
		struct fp_ring *e, struct fp_ring *q_admitted_out, struct fp_ring *f,
		struct fp_mempool *packet_mempool,
		struct fp_mempool *admitted_traffic_mempool, struct fp_ring **g,
		struct fp_ring **packet_queues, struct fp_ring **h, void *emu_args) {
	struct emu_state *emu_state = (struct emu_state *) malloc(sizeof(struct emu_state));

	emu_init_state(emu_state, admitted_traffic_mempool, q_admitted_out,
			packet_mempool, packet_queues, emu_args);
	return (struct admissible_state *) emu_state;
}

#endif

#endif /* ADMISSIBLE_H_ */
