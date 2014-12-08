/*
 * circular_queue.h
 */

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <assert.h>
#include <stdlib.h>

/**
 * @param head: index of the first item in the queue
 * @param tail: index following the last item in the queue
 * @param mask: the number of allowed elements (which is a power of two) minus 1
 * @param elem: buffer to hold pointers to packets
 */
struct circular_queue {
	uint32_t head;
	uint32_t tail;
	uint32_t mask;
	void *elem[0];
};

/**
 * Returns the amount of memory to be allocated for a circular queue with
 *   num_elems.
 * @important: num_elems must be a power of two
 * @returns the number of bytes the circular_queue must hold
 */
static inline
uint32_t cq_memsize(uint32_t num_elems)
{
	assert((num_elems & (num_elems - 1)) == 0);

	uint32_t elem_offset = (uint32_t)(&((struct circular_queue *)0)->elem[0]);
	return elem_offset + (sizeof(void *) * num_elems);
}


/**
 * Creates a new packet queue with capacity @num_elems.
 * @important: @num_elems must be a power of two
 * @param q: a chunk of memory of at least cq_memsize(num_elems) bytes
 */
static inline
void cq_init(struct circular_queue *q, uint32_t num_elems)
{
	assert((num_elems & (num_elems - 1)) == 0);
	assert(q != NULL);

	q->head = 0;
	q->tail = 0;
	q->mask = num_elems - 1;
}

/**
 * Enqueues @x at the end of @q.
 * @assumes there is enough space in q. (user can check with cq_full())
 */
static inline
void cq_enqueue(struct circular_queue *q, void *x)
{
	assert(q != NULL);
	q->elem[(q->tail++) & q->mask] = x;
}

/**
 * Dequeues an element from @q.
 * @assumes q is non-empty (user can check with cq_empty())
 */
static inline
void *cq_dequeue(struct circular_queue *q)
{
	assert(q != NULL);
	return q->elem[(q->head++) & q->mask];
}

/**
 * Returns 1 if @q is empty, 0 otherwise.
 */
static inline
int cq_empty(struct circular_queue *q)
{
	assert(q != NULL);
	return (q->tail == q->head);
}

/**
 * Returns the current occupancy of the queue.
 */
static inline
uint32_t cq_occupancy(struct circular_queue *q)
{
	assert(q != NULL);
	return q->tail - q->head;
}

/**
 * Returns 1 if @q is full, 0 otherwise.
 */
static inline
uint32_t cq_full(struct circular_queue *q)
{
	return (cq_occupancy(q) >= q->mask + 1);
}


#endif /* CIRCULAR_QUEUE_H */
