#include "stress_test_core.h"

#include "comm_core.h"
#include <rte_log.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_memcpy.h>
#include <rte_string_fns.h>
#include <rte_errno.h>
#include <rte_mempool.h>
#include <ccan/list/list.h>
#include "control.h"
#include "main.h"
#include "../graph-algo/admissible.h"
#include "../graph-algo/generate_requests.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "../protocol/stat_print.h"
#include "../protocol/topology.h"
#include "comm_log.h"
#include "admission_log.h"

#define STRESS_TEST_MIN_LOOP_TIME_SEC		1e-6
#define STRESS_TEST_RECORD_ADMITTED_INTERVAL_SEC	1
#define STRESS_TEST_BACKLOG_TOLERANCE			1.05
#define STRESS_TEST_RATE_INCREASE               1
#define STRESS_TEST_RATE_MAINTAIN               2
#define STRESS_TEST_RATE_DECREASE               3
#define STRESS_TEST_MIN_RATE_CHECK_TIME_SEC		1e-2
#define STRESS_TEST_RATE_DECREASE_GAP_SEC		1

/* logs */
struct stress_test_log {
	uint64_t processed_tslots;
	uint64_t occupied_node_tslots;
};
struct stress_test_log stress_test_core_logs[RTE_MAX_LCORE];
/* current log */
#define CL		(&stress_test_core_logs[rte_lcore_id()])

static inline void stress_test_log_init(struct stress_test_log *cl)
{
	memset(cl, 0, sizeof(*cl));
}

static inline void stress_test_log_got_admitted_tslot(uint16_t size, uint64_t timeslot) {
	(void)size;(void)timeslot;
	CL->processed_tslots++;
	CL->occupied_node_tslots += size;
}

/* process allocated traffic and return the number of MTUs admitted */
static inline uint64_t process_allocated_traffic_one_q(
		struct comm_core_state *core, struct rte_ring *q_admitted,
		struct rte_mempool *admitted_traffic_mempool)
{
	int rc;
	int i;
	struct admitted_traffic* admitted[STRESS_TEST_MAX_ADMITTED_PER_LOOP];
	uint16_t partition;
	uint64_t current_timeslot;
	uint64_t num_admitted = 0;

	/* Process newly allocated timeslots */
	rc = rte_ring_dequeue_burst(q_admitted, (void **) &admitted[0],
			STRESS_TEST_MAX_ADMITTED_PER_LOOP);
	if (unlikely(rc < 0)) {
		/* error in dequeuing.. should never happen?? */
		comm_log_dequeue_admitted_failed(rc);
		return 0;
	}

	for (i = 0; i < rc; i++) {
		partition = get_admitted_partition(admitted[i]);
		current_timeslot = ++core->latest_timeslot[partition];
		comm_log_got_admitted_tslot(get_num_admitted(admitted[i]),
				current_timeslot, partition);
		num_admitted += get_num_admitted(admitted[i]);
#if defined(EMULATION_ALGO)
		struct emu_admitted_traffic *emu_admitted;
		emu_admitted = (struct emu_admitted_traffic *) admitted[i];
		comm_log_dropped_tslots(emu_admitted->dropped);
#endif
	}
	/* free memory */
	rte_mempool_put_bulk(admitted_traffic_mempool, (void **) admitted, rc);

	return num_admitted;
}

static inline uint64_t process_allocated_traffic(struct comm_core_state *core,
		struct rte_ring **q_admitted,
		struct rte_mempool *admitted_traffic_mempool) {
#ifdef EMULATION_ALGO
	uint16_t i;
	uint64_t total = 0;
	for (i = 0; i < ALGO_N_CORES; i++)
		total += process_allocated_traffic_one_q(core, q_admitted[i],
				admitted_traffic_mempool);
	return total;
#else
	return process_allocated_traffic_one_q(core, q_admitted[0],
			admitted_traffic_mempool);
#endif
}

/**
 * Adds demands from 'num_srcs' sources, each to 'num_dsts_per_src'.
 *    Demand is for 'flow_size' tslots.
 */
static void add_initial_requests(struct comm_core_state *core,
		uint32_t num_srcs, uint32_t num_dsts_per_src, uint32_t flow_size)
{
	uint32_t src;
	uint32_t i;
	for (src = 0; src < num_srcs; src++)
		for (i = 0; i < num_dsts_per_src; i++)
			add_backlog(g_admissible_status(), src, (src + 1 + i) % num_srcs,
					flow_size, 0, NULL);

	flush_backlog(g_admissible_status());
}

void exec_stress_test_core(struct stress_test_core_cmd * cmd,
		uint64_t first_time_slot)
{
	int i;
	const unsigned lcore_id = rte_lcore_id();
	uint64_t now;
	struct request_generator gen;
	struct request next_request;
	struct comm_core_state *core = &ccore_state[lcore_id];
	uint64_t min_next_iteration_time;
	uint64_t loop_minimum_iteration_time =
			rte_get_timer_hz() * STRESS_TEST_MIN_LOOP_TIME_SEC;
	uint64_t next_rate_increase_time;
	double next_mean_t_btwn_requests;
	double cur_increase_factor;
	uint64_t last_successful_mean_t;
	uint64_t prev_node_tslots = 0;
	uint64_t cur_node_tslots = 0;
	uint64_t max_node_tslots = 0;
	bool re_init_gen = true;
	uint64_t admitted_tslots[STRESS_TEST_DURATION_SEC /
	                         STRESS_TEST_RECORD_ADMITTED_INTERVAL_SEC];
	uint64_t next_record_admitted_time, next_rate_check_time;
	uint64_t min_rate_check_interval = rte_get_timer_hz() *
			STRESS_TEST_MIN_RATE_CHECK_TIME_SEC;
	uint32_t admitted_index = 0;
	uint64_t total_demand, total_occupied_node_tslots = 0;
	uint64_t prev_demand, prev_occupied_node_tslots = 0;
	uint16_t lcore;
	uint64_t core_ahead_prev[RTE_MAX_LCORE];
	bool persistently_behind = false;
	uint64_t next_rate_decrease_time;
	uint32_t percent_out_of_group; (void) percent_out_of_group;

	for (i = 0; i < N_PARTITIONS; i++)
		core->latest_timeslot[i] = first_time_slot - 1;
	stress_test_log_init(&stress_test_core_logs[lcore_id]);
	comm_log_init(&comm_core_logs[lcore_id]);

	/* Add initial demands */
	assert(cmd->num_initial_srcs <= cmd->num_nodes);
//	assert(cmd->num_initial_dsts_per_src < cmd->num_initial_srcs);
	add_initial_requests(core, cmd->num_initial_srcs,
			cmd->num_initial_dsts_per_src, cmd->initial_flow_size);

	/* Initialize gen */
	next_mean_t_btwn_requests = cmd->mean_t_btwn_requests;
	last_successful_mean_t = next_mean_t_btwn_requests;
	cur_increase_factor = STRESS_TEST_RATE_INCREASE_FACTOR;
	comm_log_stress_test_increase_factor(cur_increase_factor);

	if (STRESS_TEST_IS_AUTOMATED)
		comm_log_stress_test_mode(STRESS_TEST_RATE_MAINTAIN);

	/* determine percentage of traffic that will be out-of-group (aka out of
	 * rack) */
#if defined(EMULATION_ALGO)
	percent_out_of_group = 100 / (EMU_NUM_RACKS - 1);
#endif

	while (rte_get_timer_cycles() < cmd->start_time);

	now = rte_get_timer_cycles();
	next_rate_increase_time = now;
	next_record_admitted_time = now;
	next_rate_check_time = now;

	init_request_generator(&gen, next_mean_t_btwn_requests, now,
			cmd->num_nodes, cmd->demand_tslots);

	/* generate the first request */
#if defined(EMULATION_ALGO)
	get_next_request_biased(&gen, &next_request, percent_out_of_group);
#else
	get_next_request(&gen, &next_request);
#endif

	/* MAIN LOOP */
	while (now < cmd->end_time) {
		uint32_t n_processed_requests = 0;
		re_init_gen = false;

		/* check rate periodically but not every iteration of main loop (to
		 * smooth out variation in when requests arrive, etc.) */
		if (now > next_rate_check_time) {

			bool fell_behind = false;
#if defined(EMULATION_ALGO)
			for (i = 0; i < N_ADMISSION_CORES; i++) {
				lcore = enabled_lcore[FIRST_ADMISSION_CORE + i];
				struct admission_log *al = &admission_core_logs[lcore];
				fell_behind |= (al->core_ahead == core_ahead_prev[lcore]);
				core_ahead_prev[lcore] = al->core_ahead;
			}
#else
			fell_behind = (((total_occupied_node_tslots - prev_occupied_node_tslots) *
					STRESS_TEST_BACKLOG_TOLERANCE) < (total_demand - prev_demand));
			prev_occupied_node_tslots = total_occupied_node_tslots;
			prev_demand = total_demand;
#endif

			if (STRESS_TEST_IS_AUTOMATED && fell_behind) {
				if (comm_log_get_stress_test_mode() ==
						STRESS_TEST_RATE_INCREASE) {
					/* rate increase was unsuccessful, decrease the rate change
					 * factor */
					cur_increase_factor = (cur_increase_factor + 1) / 2;
					comm_log_stress_test_increase_factor(cur_increase_factor);
				}

				/* When the emulator falls behind, quickly return to the last
				 * rate for which it could keep up (don't wait for the next
				 * rate increase time). If we are already at that last rate
				 * and it remains behind for one second, further decrease the
				 * rate. */
				if (next_mean_t_btwn_requests < last_successful_mean_t) {
					/* go back to last successful mean time */
					re_init_gen = true;
					next_mean_t_btwn_requests = last_successful_mean_t;
					comm_log_stress_test_mode(STRESS_TEST_RATE_MAINTAIN);
				} else if (next_mean_t_btwn_requests <
						cmd->mean_t_btwn_requests) {
					if (persistently_behind &&
							(now >= next_rate_decrease_time)) {
						/* persistently behind, decrease rate by the current
						 * increase factor */
						re_init_gen = true;
						next_mean_t_btwn_requests *= cur_increase_factor;
						comm_log_stress_test_mode(STRESS_TEST_RATE_DECREASE);
					} else if (!persistently_behind) {
						persistently_behind = true;
						next_rate_decrease_time = now + rte_get_timer_hz() *
								STRESS_TEST_RATE_DECREASE_GAP_SEC;
					}
				}
			}

			/* completed an interval. time to adjust the rate. */
			if (now >= next_rate_increase_time && !re_init_gen) {
				prev_node_tslots = cur_node_tslots;
				cur_node_tslots = total_occupied_node_tslots;
				if (STRESS_TEST_IS_AUTOMATED && cur_node_tslots != 0) {
					/* did successfully allocate the offered demand */
					last_successful_mean_t = next_mean_t_btwn_requests;
					next_mean_t_btwn_requests /= cur_increase_factor;
					comm_log_stress_test_mode(STRESS_TEST_RATE_INCREASE);

					/* log the node tslots achieved in this interval */
					uint64_t node_tslots_in_interval = cur_node_tslots
							- prev_node_tslots;
					if (node_tslots_in_interval > max_node_tslots) {
						max_node_tslots = node_tslots_in_interval;
						comm_log_stress_test_max_node_tslots(max_node_tslots);
					}
				} else if (!STRESS_TEST_IS_AUTOMATED) {
					next_mean_t_btwn_requests /= STRESS_TEST_RATE_INCREASE_FACTOR;
				}
				re_init_gen = true;
			}

			if (re_init_gen) {
				/* reinitialize the request generator */
				comm_log_mean_t(next_mean_t_btwn_requests);
				reinit_request_generator(&gen, next_mean_t_btwn_requests, now,
						cmd->num_nodes, cmd->demand_tslots);
#if defined(EMULATION_ALGO)
				get_next_request_biased(&gen, &next_request,
						percent_out_of_group);
#else
				get_next_request(&gen, &next_request);
#endif

				next_rate_increase_time = rte_get_timer_cycles() +
						rte_get_timer_hz() * STRESS_TEST_RATE_INCREASE_GAP_SEC;

				/* update node tslots */
				cur_node_tslots = total_occupied_node_tslots;
				persistently_behind = false;
			}
			next_rate_check_time += min_rate_check_interval;
		}

		/* if time to enqueue request, do so now */
		for (i = 0; i < MAX_ENQUEUES_PER_LOOP; i++) {
			if (next_request.time > now)
				break;

			/* enqueue the request */
			add_backlog(g_admissible_status(), next_request.src,
					next_request.dst, next_request.backlog, 0, NULL);
			comm_log_demand_increased(next_request.src, next_request.dst, 0,
					next_request.backlog, next_request.backlog);
			total_demand += next_request.backlog;

			n_processed_requests++;

			/* generate the next request */
#if defined(EMULATION_ALGO)
			get_next_request_biased(&gen, &next_request, percent_out_of_group);
#else
			get_next_request(&gen, &next_request);
#endif
		}

		comm_log_processed_batch(n_processed_requests, now);

		/* Process newly allocated timeslots */
		total_occupied_node_tslots += process_allocated_traffic(core,
				cmd->q_allocated, cmd->admitted_traffic_mempool);

		/* Process the spent demands, launching a new demand for demands where
		 * backlog increased while the original demand was being allocated */
		handle_spent_demands(g_admissible_status());

		/* flush q_head's buffer into q_head */
		flush_backlog(g_admissible_status());

		/* record the cumulative number of tslots allocated at each second
		 * throughout the experiment */
		if (next_record_admitted_time <= now) {
			admitted_tslots[admitted_index++] = total_occupied_node_tslots;
			next_record_admitted_time += rte_get_timer_hz() *
					STRESS_TEST_RECORD_ADMITTED_INTERVAL_SEC;
		}

		/* wait until at least loop_minimum_iteration_time has passed from
		 * beginning of loop */
		min_next_iteration_time = now + loop_minimum_iteration_time;
		do {
			now = rte_get_timer_cycles();
			comm_log_stress_test_ahead();
		} while (now < min_next_iteration_time);
	}

stress_test_done:
	/* Dump some stats */
	printf("Stress test finished\n");

	if (admitted_index > 30) {
		double tput_gbps = (admitted_tslots[admitted_index - 1] -
				admitted_tslots[admitted_index - 31]) * 1500 * 8 /
						((double) 30 * 1000 * 1000 * 1000);
		printf("Throughput over last 30 seconds: %f\n", tput_gbps);
	} else if (admitted_index > 10) {
		double tput_gbps = (admitted_tslots[admitted_index - 1] -
				admitted_tslots[admitted_index - 11]) * 1500 * 8 /
						((double) 10 * 1000 * 1000 * 1000);
		printf("Ran for less than 30 seconds. Throughput over last 10 seconds: %f\n",
				tput_gbps);
	}
	rte_exit(0, "Done!\n");
}
