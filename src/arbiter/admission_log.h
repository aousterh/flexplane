/*
 * admission_log.h
 *
 *  Created on: Jan 2, 2014
 *      Author: yonch
 */

#ifndef ADMISSION_LOG_H_
#define ADMISSION_LOG_H_

#include <rte_log.h>
#include <rte_lcore.h>
#include <rte_byteorder.h>
#include <rte_cycles.h>

#include "control.h"
#include "../graph-algo/batch.h"
#include "dpdk-time.h"

#define		MAINTAIN_AFTER_TSLOTS_HISTOGRAM		1
#define		AFTER_TSLOTS_HISTOGRAM_NUM_BINS		32
#define		AFTER_TSLOTS_HISTOGRAM_SHIFT		0
#define		TSLOTS_BEHIND_TOLERANCE				50

/**
 * logged information for a core
 * @core_ahead: this core was ahead of schedule for this many timeslots
 * @core_behind: this core was behind for this many timeslots
 */
struct admission_log {
	uint64_t batches_started;
	uint64_t last_started_alloc_tsc;
	uint64_t after_tslots_histogram[AFTER_TSLOTS_HISTOGRAM_NUM_BINS];
	uint64_t core_ahead;
	uint64_t core_behind;
	uint64_t tslots_skipped;
};

extern struct admission_log admission_core_logs[RTE_MAX_LCORE];

#define RTE_LOGTYPE_ADMISSION RTE_LOGTYPE_USER1

#ifdef CONFIG_IP_FASTPASS_DEBUG
#define ADMISSION_DEBUG(a...) RTE_LOG(DEBUG, ADMISSION, ##a)
#else
#define ADMISSION_DEBUG(a...)
#endif

/* current comm log */
#define AL		(&admission_core_logs[rte_lcore_id()])

static inline void admission_log_init(struct admission_log *al)
{
	memset(al, 0, sizeof(*al));
}

static inline
void admission_log_allocation_begin(uint64_t current_timeslot) {
	(void)current_timeslot;
	uint64_t now = rte_get_tsc_cycles();
	AL->last_started_alloc_tsc = now;
	AL->batches_started++;
//	ADMISSION_DEBUG("core %d started allocation of batch %lu (timeslot %lu cycle timer %lu)\n",
//			rte_lcore_id(), AL->batches_started, current_timeslot, now);
}

static inline void admission_log_allocation_end(uint64_t logical_timeslot) {
//	uint64_t now = rte_get_tsc_cycles();
//	ADMISSION_DEBUG("core %d finished allocation of batch %lu (cycle timer %lu, took %lu)\n",
//			rte_lcore_id(), AL->batches_started, now,
//			now - AL->last_started_alloc_tsc);
	if (MAINTAIN_AFTER_TSLOTS_HISTOGRAM) {
		uint32_t hist_bin;
		uint64_t now_timeslot = (fp_get_time_ns() * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
		uint64_t gap = now_timeslot - (logical_timeslot + BATCH_SIZE - 1);

		/* mainatain histogram */
		hist_bin = gap >> AFTER_TSLOTS_HISTOGRAM_SHIFT;
		if (unlikely(hist_bin >= AFTER_TSLOTS_HISTOGRAM_NUM_BINS))
			hist_bin = AFTER_TSLOTS_HISTOGRAM_NUM_BINS - 1;
		AL->after_tslots_histogram[hist_bin]++;
	}
}

static inline void admission_log_core_ahead() {
	AL->core_ahead++;
}

static inline void admission_log_core_skipped_tslots(uint64_t tslots_skipped) {
	AL->core_behind++;
	AL->tslots_skipped += tslots_skipped;
}

#undef CL


#endif /* ADMISSION_LOG_H_ */
