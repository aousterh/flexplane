/*
 * queue_bank_log.h
 *
 *  Created on: January 9, 2014
 *      Author: aousterh
 */

#ifndef QUEUE_BANK_LOG_H_
#define QUEUE_BANK_LOG_H_

#include "../protocol/platform/generic.h"

#define MAINTAIN_QUEUE_BANK_LOG_COUNTERS	1
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
};

struct port_drop_stats {
	u64		port_drops[QUEUE_BANK_MAX_PORTS];
	u64		port_marks[QUEUE_BANK_MAX_PORTS];
	u64		total_drops;
	u64		total_marks;
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
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->total_drops++;
}

static inline __attribute__((always_inline))
void queue_bank_log_mark(struct port_drop_stats *st, uint32_t port) {
	if (MAINTAIN_QUEUE_BANK_LOG_COUNTERS)
		st->port_marks[port]++;
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->total_marks++;
}

/**
 * Print the current contents of the queue bank log to a specific file.
 */
static inline
void print_queue_bank_log_to_file(FILE *fp, struct queue_bank_stats *st,
		struct port_drop_stats *pdst, u64 time_ns)
{
	uint32_t port;

	/* Take snapshot so we can print without values changing underneath us */
	struct queue_bank_stats lst;
	struct port_drop_stats lpdst;
	memcpy(&lst, st, sizeof(lst));
	memcpy(&lpdst, pdst, sizeof(lst));

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
		fprintf(fp, ",%llu", lpdst.port_drops[port]);
	}
	fprintf(fp, "\n%llu,marks", time_ns);
	for (port = 0; port < QUEUE_BANK_MAX_PORTS; port++) {
		fprintf(fp, ",%llu", lpdst.port_marks[port]);
	}
	fprintf(fp, "\n");
}

/**
 * Print the current contents of the queue bank log to standard out.
 */
static inline
void print_queue_bank_log(struct queue_bank_stats *st,
		struct port_drop_stats *pdst, uint64_t time_ns)
{
  print_queue_bank_log_to_file(stdout, st, pdst, time_ns);
}

#endif /* QUEUE_BANK_LOG_H_ */
