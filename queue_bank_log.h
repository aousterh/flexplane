/*
 * queue_bank_log.h
 *
 *  Created on: January 9, 2014
 *      Author: aousterh
 */

#ifndef QUEUE_BANK_LOG_H_
#define QUEUE_BANK_LOG_H_

#define MAINTAIN_QUEUE_BANK_LOG_COUNTERS	0
#define QUEUE_BANK_MAX_PORTS				64

/**
 * Queue bank stats at one time - enqueues, dequeues, and drops
 * @ port_enqueues: number of enqueues that have occurred at each port
 * @ port_dequeues: number of dequeues that have occurred at each port
 * @ port_drops: number of drops that have occurred at each port
 */
struct queue_bank_stats {
	uint64_t port_enqueues[QUEUE_BANK_MAX_PORTS];
	uint64_t port_dequeues[QUEUE_BANK_MAX_PORTS];
	uint64_t port_drops[QUEUE_BANK_MAX_PORTS];
};

static inline __attribute__((always_inline))
void queue_bank_log_enqueue(struct queue_bank_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS && st != NULL)
		st->port_enqueues[port]++;
}

static inline __attribute__((always_inline))
void queue_bank_log_dequeue(struct queue_bank_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS && st != NULL)
		st->port_dequeues[port]++;
}

static inline __attribute__((always_inline))
void queue_bank_log_drop(struct queue_bank_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS && st != NULL)
		st->port_drops[port]++;
}

/**
 * Print the current contents of the queue bank log.
 */
static inline
void print_queue_bank_log(struct queue_bank_stats *st) {
	uint32_t port;

	printf("\nenqueues:   ");
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		printf(" %llu", st->port_enqueues[port]);
	}
	printf("\ndequeues:   ");
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		printf(" %llu", st->port_dequeues[port]);
	}
	printf("\noccupancies:");
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		printf(" %llu", st->port_enqueues[port] - st->port_dequeues[port]);
	}
	printf("\ndrops:      ");
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		printf(" %llu", st->port_drops[port]);
	}
	printf("\n");
}

#endif /* QUEUE_BANK_LOG_H_ */
