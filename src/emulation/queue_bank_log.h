/*
 * queue_bank_log.h
 *
 *  Created on: January 9, 2014
 *      Author: aousterh
 */

#ifndef QUEUE_BANK_LOG_H_
#define QUEUE_BANK_LOG_H_

#include "../protocol/platform/generic.h"

#define MAINTAIN_QUEUE_BANK_LOG_COUNTERS	0
#define QUEUE_BANK_MAX_PORTS				64

/**
 * Queue bank stats at one time - enqueues, dequeues, and drops
 * @ port_enqueues: number of enqueues that have occurred at each port
 * @ port_dequeues: number of dequeues that have occurred at each port
 */
struct queue_bank_stats {
	u64		port_enqueues[QUEUE_BANK_MAX_PORTS];
	u64		port_dequeues[QUEUE_BANK_MAX_PORTS];
};

/**
 * Port drop stats at one time - drops and marks
 * @ port_drops: number of drops that have occurred at each port
 * @ port_marks: number of marks that have occurred at each port
 */
struct port_drop_stats {
	u64		port_drops[QUEUE_BANK_MAX_PORTS];
	u64		port_marks[QUEUE_BANK_MAX_PORTS];
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
void queue_bank_log_drop(struct port_drop_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS)
		st->port_drops[port]++;
}

static inline __attribute__((always_inline))
void queue_bank_log_mark(struct port_drop_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS)
		st->port_marks[port]++;
}

/**
 * Print the current contents of the queue bank stats to a specific file.
 */
static inline
void print_queue_bank_stats_to_file(FILE *fp, struct queue_bank_stats *st,
		u64 time_ns)
{
	uint32_t port;

	/* Take snapshot so we can print without values changing underneath us */
	struct queue_bank_stats lst;
	memcpy(&lst, st, sizeof(lst));

	fprintf(fp, "%llu,enqueues", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lst.port_enqueues[port]);
	}
	fprintf(fp, "\n%llu,dequeues", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lst.port_dequeues[port]);
	}
	fprintf(fp, "\n%llu,occupancies", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%lld", lst.port_enqueues[port] - lst.port_dequeues[port]);
	}
	fprintf(fp, "\n");
}

/**
 * Print the current contents of the port drop stats to a specific file.
 */
static inline
void print_port_drop_stats_to_file(FILE *fp, struct port_drop_stats *pdst,
		u64 time_ns)
{
	uint32_t port;

	/* Take snapshot so we can print without values changing underneath us */
	struct port_drop_stats lpdst;
	memcpy(&lpdst, pdst, sizeof(lpdst));

	fprintf(fp, "%llu,drops", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lpdst.port_drops[port]);
	}
	fprintf(fp, "\n%llu,marks", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lpdst.port_marks[port]);
	}
	fprintf(fp, "\n");
}

#endif /* QUEUE_BANK_LOG_H_ */
