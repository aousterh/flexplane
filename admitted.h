/*
 * admitted.h
 *
 *  Created on: July 7, 2014
 *      Author: aousterh
 */

#ifndef ADMITTED_H_
#define ADMITTED_H_

#define NO_FLAGS     0
#define DROP         1

#include "config.h"

#include <assert.h>

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
    struct admitted_edge edges[EMU_NUM_ENDPOINTS];
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
    edge->should_drop = should_drop;
}

#endif /* ADMITTED_H_ */
