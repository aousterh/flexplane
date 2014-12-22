#ifndef GRAPH_ALGO_RANDOM_H_
#define GRAPH_ALGO_RANDOM_H_

#include "../protocol/platform/generic.h"

/**
 * Use Numerical Recipes
 * http://en.wikipedia.org/wiki/Linear_congruential_generator
 */
#define GA_RAND_A		1664525
#define GA_RAND_C		1013904223

static inline __attribute__((always_inline))
void seed_random(u32 *state, u32 value)
{
	*state = value;
}

/* produces a value between 0 and max_val */
static inline __attribute__((always_inline))
u32 random_int(u32 *state, u16 max_val)
{
	u32 res = ((*state >> 16) * max_val) >> 16;
	*state = *state * GA_RAND_A + GA_RAND_C;
        return res;
}

#endif /* GRAPH_ALGO_RANDOM_H_ */

