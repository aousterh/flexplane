/*
 * admitted.h
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#ifndef EMU_ADMITTED_H_
#define EMU_ADMITTED_H_

#include "config.h"
#include "admissible_log.h"
#include "packet.h"
#include "../protocol/flags.h"
#include "../protocol/topology.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#define EMU_ADMITS_PER_ADMITTED	128

struct emu_admitted_edge {
	uint16_t	src;
	uint16_t	dst;
	uint8_t		flags; /* only 4 lowest bits are used */
	uint16_t	id; /* sequential id within this flow */
	uint8_t		data[MAX_ALLOC_DATA_BYTES];
};

/**
 * Struct to hold admitted edges when passed from timeslot allocation cores to
 * communication cores. Holds both admitted and dropped edges.
 */
struct emu_admitted_traffic {
	uint16_t size;
	uint16_t admitted;
	uint16_t dropped;
	struct emu_admitted_edge edges[EMU_ADMITS_PER_ADMITTED];
};

/**
 * Initialize admitted traffic to empty
 */
static inline
void admitted_init(struct emu_admitted_traffic *admitted) {
	assert(admitted != NULL);

	admitted->size = 0;
	admitted->admitted = 0;
	admitted->dropped = 0;
}

/**
 * For each algorithm, copy whatever data should be returned to the endpoint
 * into the data field of the admitted edge.
 */
static inline
void copy_alloc_data_to_admitted_edge(struct emu_packet *packet,
		struct emu_admitted_edge *edge) {
	/* TODO */
}

/**
 * Add an edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_edge(struct emu_admitted_traffic *admitted,
		struct emu_packet *packet, uint16_t flags,
		struct emu_admission_core_statistics *stat) {
	assert(admitted != NULL);
	assert(packet->src < EMU_NUM_ENDPOINTS);
	assert(packet->dst < EMU_NUM_ENDPOINTS);
	assert(admitted->size < EMU_ADMITS_PER_ADMITTED);

	struct emu_admitted_edge *edge = &admitted->edges[admitted->size++];
	edge->src = packet->src;
	/* indicate dst as combination of dst_node and flow */
	edge->dst = (packet->dst << FLOW_SHIFT) | (packet->flow & FLOW_MASK);
	edge->flags = flags & FLAGS_MASK;
	edge->id = packet->id;

	copy_alloc_data_to_admitted_edge(packet, edge);
}

/**
 * Add an admitted edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_admitted_edge(struct emu_admitted_traffic *admitted,
		struct emu_packet *packet,
		struct emu_admission_core_statistics *stat) {
//	assert(admitted->admitted < EMU_NUM_ENDPOINTS);

	/* pass flags to admitted struct */
	admitted_insert_edge(admitted, packet, packet->flags, stat);
	admitted->admitted++;
}

/**
 * Add a dropped edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_dropped_edge(struct emu_admitted_traffic *admitted,
		struct emu_packet *packet,
		struct emu_admission_core_statistics *stat) {
	/* ignore any flags the packet has - mark as a DROP */
	admitted_insert_edge(admitted, packet, EMU_FLAGS_DROP, stat);
	admitted->dropped++;
}

/**
 * Print out one admitted edge, for debugging/testing
 */
static inline
void admitted_edge_print(struct emu_admitted_edge *edge) {
	uint16_t dst, flow;

	dst = edge->dst >> FLOW_SHIFT;
	flow = edge->dst & FLOW_MASK;
	if (edge->flags == EMU_FLAGS_DROP) {
		printf("\tDROP src %d to dst %d (flow %d, id %d)\n", edge->src, dst,
				flow, edge->id);
	} else if (edge->flags == EMU_FLAGS_ECN_MARK) {
		printf("\tMARK src %d to dst %d (flow %d, id %d)\n", edge->src, dst,
				flow, edge->id);
	} else {
		printf("\tsrc %d to dst %d (flow %d, id %d)\n", edge->src, dst, flow,
				edge->id);
	}
}

/**
 * Print out all edges in an admitted struct, for debugging/testing
 */
static inline
void admitted_print(struct emu_admitted_traffic *admitted) {
	uint16_t i;

	printf("finished traffic:\n");
	for (i = 0; i < admitted->size; i++)
		admitted_edge_print(&admitted->edges[i]);
}

/**
 * Returns the size of an admitted struct.
 */
static inline
uint32_t get_admitted_struct_size() {

	return sizeof(struct emu_admitted_traffic);
}

#endif /* EMU_ADMITTED_H_ */
