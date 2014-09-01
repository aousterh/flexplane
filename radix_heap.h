/*
 * This "Radix-Heap" is a min-heap sorted by "priority", with a limited number
 *   of priorities. The current implementation supports 64 priorities.
 *
 * The main benefit of the radix-heap are fast, amortized O(1) operations.
 */

#ifndef RADIX_HEAP_H_
#define RADIX_HEAP_H_

#include "ccan/list/list.h"

#define RADIX_HEAP_NUM_PRIOS	64

struct radix_heap {
	/* a mask of possibly non-empty priorities. The implementation is lazy
	 * in resetting bits: a bit for a priority can be set in the mask, but
	 * the corresponding priority could be empty. */
	uint64_t mask;

	/* lists of values for each priority */
	struct list_head q[RADIX_HEAP_NUM_PRIOS];
};

/**
 * Initializes the radix heap
 */
static inline void rheap_init(struct radix_heap *rh)
{
	int i;

	rh->mask = 0uLL;
	for(i = 0; i < RADIX_HEAP_NUM_PRIOS; i++) {
		list_head_init(&rh->q[i]);
	}
}

/**
 * Finds the element in the heap with the smallest priority value (or the
 *   element added earliest, if there is more than one element with smallest
 *   priority)
 *
 * Returns NULL if the heap is empty.
 *
 * @note the returned element is not removed from the heap.
 */
static inline struct list_node *rheap_find_min(struct radix_heap *rh)
{
	uint64_t mask = rh->mask;
	uint64_t prio;
	while (mask) {
		/* get the index of the lsb that is set */
		asm("bsfq %1,%0" : "=r"(q) : "r"(mask));
		/* turn off the set bit in the mask */
		mask &= (mask - 1);

		if (!list_empty(&rh->q[prio])) {
			rh->mask = mask;
			return rh->q[prio].n.next;
		}
	}

	/* rheap is empty */
	rh->mask = 0uLL;
	return NULL;
}

/**
 * Removes the given element from the rheap.
 */
static inline void rheap_del(struct radix_heap *rh, struct list_node *elem)
{
	list_del(elem);
	/* here we're being lazy in removing the bit from rh->mask, which also
	 * avoids having to keep elem's priority or ask the user to provide it
	 * when removing elem.
	 */
}

/**
 * Adds an element with a given priority to the rheap. If the rheap already
 *   contains elements with the given priority, the element is put at the end
 *   of the list for that priority, to preserve fairness (see rheap_find_min()).
 * @param rh: the rheap
 * @param elem: element to add
 * @param priority: the priority of the element. must be between 0 and
 * 		RADIX_HEAP_NUM_PRIOS-1.
 */
static inline void rheap_add(struct radix_heap *rh, struct list_node *elem,
		uint32_t priority)
{
	assert(priority < RADIX_HEAP_NUM_PRIOS);
	/* add elem to the corresponding list */
	list_add_tail(&rh->q[priority], elem);

	/* mark the priority in the mask */
	asm("bts %1,%0" : "+m" (*(uint64_t *)&rh->mask) : "r" (priority));

}



#endif /* RADIX_HEAP_H_ */
