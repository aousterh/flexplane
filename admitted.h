/*
 * admitted.h
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#ifndef EMU_ADMITTED_H_
#define EMU_ADMITTED_H_

#include "config.h"
#include "../protocol/encoding.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#define FLAGS_NONE	0
#define FLAGS_DROP	0x8000

struct emu_admitted_edge {
	uint16_t src;
	uint16_t dst;
	uint16_t flags;
};

/**
 * Struct to hold admitted edges when passed from timeslot allocation cores to
 * communication cores. Holds both admitted and dropped edges.
 */
struct emu_admitted_traffic {
	uint16_t size;
	uint16_t admitted;
	uint16_t dropped;
	struct emu_admitted_edge edges[EMU_NUM_ENDPOINTS + EMU_MAX_DROPS];
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
 * Add an edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_edge(struct emu_admitted_traffic *admitted, uint16_t src,
			  uint16_t dst, uint16_t flags) {
	assert(admitted != NULL);
	assert(admitted->size < EMU_NUM_ENDPOINTS + EMU_MAX_DROPS);
	assert(src < EMU_NUM_ENDPOINTS);
	assert(dst < EMU_NUM_ENDPOINTS);

	struct emu_admitted_edge *edge = &admitted->edges[admitted->size++];
	edge->src = src;
	edge->dst = dst;
	edge->flags = flags;
}

/**
 * Add an admitted edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_admitted_edge(struct emu_admitted_traffic *admitted,
				   uint16_t src, uint16_t dst) {
	assert(admitted->admitted < EMU_NUM_ENDPOINTS);

	admitted_insert_edge(admitted, src, dst, FLAGS_NONE);
	admitted->admitted++;
}

/**
 * Add a dropped edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_dropped_edge(struct emu_admitted_traffic *admitted,
				  uint16_t src, uint16_t dst) {
	assert(admitted->dropped < EMU_MAX_DROPS);

	admitted_insert_edge(admitted, src, dst, FLAGS_DROP);
	admitted->dropped++;
}

/**
 * Print out one admitted edge, for debugging/testing
 */
static inline
void admitted_edge_print(struct emu_admitted_edge *edge) {
	if (edge->flags & FLAGS_DROP) {
		printf("\tDROP src %d to dst %d\n", edge->src, edge->dst);
	} else {
		printf("\tsrc %d to dst %d\n", edge->src, edge->dst);
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
