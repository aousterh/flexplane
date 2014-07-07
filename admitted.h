/*
 * admitted.h
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#ifndef ADMITTED_H_
#define ADMITTED_H_

#define FLAGS_NONE     0
#define FLAGS_DROP     1

#include "config.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

struct emu_admitted_edge {
        uint16_t src;
        uint16_t dst;
        uint16_t id;
        uint16_t flags;
};

/**
 * Struct to hold admitted edges when passed from timeslot allocation cores to
 * communication cores
 */
struct emu_admitted_traffic {
    uint16_t size;
    struct emu_admitted_edge edges[EMU_NUM_ENDPOINTS];
};

/**
 * Initialize admitted traffic to empty
 */
static inline
void admitted_init(struct emu_admitted_traffic *admitted) {
    assert(admitted != NULL);

    admitted->size = 0;
}

/**
 * Add an admitted edge to the admitted struct
 */
static inline __attribute__((always_inline))
void admitted_insert_edge(struct emu_admitted_traffic *admitted, uint16_t src,
                          uint16_t dst, uint16_t id, uint16_t flags) {
    assert(admitted != NULL);
    assert(admitted->size < EMU_NUM_ENDPOINTS);
    assert(src < EMU_NUM_ENDPOINTS);
    assert(dst < EMU_NUM_ENDPOINTS);

    struct emu_admitted_edge *edge = &admitted->edges[admitted->size++];
    edge->src = src;
    edge->dst = dst;
    edge->id = id;
    edge->flags = flags;
}

/**
 * Print out one admitted edge, for debugging/testing
 */
static inline
void admitted_edge_print(struct emu_admitted_edge *edge) {
        if (edge->flags & FLAGS_DROP) {
                printf("\tDROP src %d to dst %d (id %d)\n", edge->src,
                       edge->dst, edge->id);
        } else {
                printf("\tsrc %d to dst %d (id %d)\n", edge->src,
                       edge->dst, edge->id);
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

#endif /* ADMITTED_H_ */
