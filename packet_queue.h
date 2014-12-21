/*
 * packet_queue.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_

#include <assert.h>
#include <stdlib.h>

#define MAX_PACKET_QUEUE_LEN	128

struct emu_packet;

/**
 * A queue of packets, accessed by a single core.
 * @capacity: max items allowed in queue
 * @occupancy: current number of items in queue
 * @head: index of the first item in the queue
 * @tail: index following the last item in the queue
 * @elem: buffer to hold pointers to packets
 */
struct packet_queue {
	uint32_t capacity;
	uint32_t occupancy;
	uint32_t head;
	uint32_t tail;
	struct emu_packet *elem[MAX_PACKET_QUEUE_LEN];
        /* Useful statistics */
        uint32_t count_pkts_since_last_drop_or_mark;
        uint32_t count_pkts_dropped;
        uint32_t count_pkts_marked;
};

/**
 * Creates a new packet queue with capacity @num_elems.
 */
static inline
void queue_create(struct packet_queue *q, uint32_t num_elems) {
	assert(num_elems <= MAX_PACKET_QUEUE_LEN);
	assert(q != NULL);

	q->capacity = num_elems;
	q->occupancy = 0;
	q->head = 0;
	q->tail = 0;

        /* Statistics */
        q->count_pkts_since_last_drop_or_mark = 0;
        q->count_pkts_dropped = 0;
        q->count_pkts_marked = 0;
}

/**
 * Enqueues @packet at the end of @q, @returns 0 on success, -1 otherwise.
 */
static inline
int queue_enqueue(struct packet_queue *q, struct emu_packet *packet) {
	assert(q != NULL);

        if (q->occupancy == q->capacity)
                return -1;

	q->elem[q->tail++] = packet;
	q->occupancy++;
	if (q->tail == q->capacity)
		q->tail = 0;
        
	return 0;
}

/**
 * Dequeues a packet from @q and puts the resulting pointer in @packet_p.
 * @returns 0 on success, -1 if @q is empty.
 */
static inline
int queue_dequeue(struct packet_queue *q, struct emu_packet **packet_p) {
	assert(q != NULL);
	assert(packet_p != NULL);

	if (q->occupancy == 0)
		return -1;

	*packet_p = q->elem[q->head++];
	q->occupancy--;

	if (q->head == q->capacity)
		q->head = 0;

	return 0;
}

/**
 * Returns 1 if @q is empty, 0 otherwise.
 */
static inline
int queue_empty(struct packet_queue *q) {
	assert(q != NULL);
	return (q->occupancy == 0);
}

/**
 * Returns the current occupancy of the queue.
 */
static inline
uint32_t queue_occupancy(struct packet_queue *q) {
	assert(q != NULL);

	return q->occupancy;
}

#endif /* PACKET_QUEUE_H_ */
