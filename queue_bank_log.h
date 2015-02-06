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
 * @ port_drops: number of drops that have occurred at each port
 */
struct queue_bank_stats {
	u64		port_enqueues[QUEUE_BANK_MAX_PORTS];
	u64		port_dequeues[QUEUE_BANK_MAX_PORTS];
	u64		port_drops[QUEUE_BANK_MAX_PORTS];
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
 * Print the current contents of the queue bank log to a specific file.
 */
static inline
void print_queue_bank_log_to_file(FILE *fp, struct queue_bank_stats *st,
		u64 time_ns)
{
	uint32_t port;

	/* Take snapshot so we can print without values changin underneath us */
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
	fprintf(fp, "\n%llu,drops", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lst.port_drops[port]);
	}
	fprintf(fp, "\n");
}

/**
 * Print the current contents of the queue bank log to standard out.
 */
static inline
void print_queue_bank_log(struct queue_bank_stats *st, uint64_t time_ns) {
  print_queue_bank_log_to_file(stdout, st, time_ns);
}

#endif /* QUEUE_BANK_LOG_H_ */
