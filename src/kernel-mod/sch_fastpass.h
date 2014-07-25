/*
 * sch_fastpass.h
 *
 *  Created on: July 25, 2014
 *      Author: aousterh
 */

#ifndef SCH_FASTPASS_H_
#define SCH_FASTPASS_H_

struct fp_sched_stat {
	/* dequeue-related */
	__u64		admitted_timeslots;
	__u64		early_enqueue;
	__u64		late_enqueue1;
	__u64		late_enqueue2;
	__u64		late_enqueue3;
	__u64		late_enqueue4;

	/* request-related */
	__u64		req_alloc_errors;
	__u64		request_with_empty_flowqueue;
	__u64		queued_flow_already_acked;
	/* alloc-related */
	__u64		alloc_too_late;
	__u64		alloc_premature;
	__u64		unwanted_alloc;
	/* alloc report-related */
	__u64		alloc_report_larger_than_requested;
	__u64		timeslots_assumed_lost;
};

#endif /* SCH_FASTPASS_H_ */
