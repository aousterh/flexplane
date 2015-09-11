/*
 * log_core.c
 *
 *  Created on: Jan 14, 2014
 *      Author: yonch
 */

#include <stdio.h>

#include <rte_log.h>

#include "log_core.h"
#include "control.h"
#include "comm_core.h"
#include "comm_log.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "admission_log.h"
#include "benchmark_log.h"
#include "benchmark_log_impl.h"
#include "../emulation/admissible_log.h"
#include "../emulation/admissible_log_impl.h"
#include "../emulation/queue_bank_log.h"
#include "../grant-accept/partitioning.h"
#include "../graph-algo/admissible_algo_log.h"
#include "../graph-algo/algo_config.h"
#include "../protocol/fpproto.h"
#include "../protocol/platform.h"
#include "../protocol/stat_print.h"

#define MAX_FILENAME_LEN 256

#define RTE_LOGTYPE_LOGGING RTE_LOGTYPE_USER1
#define LOGGING_ERR(a...) RTE_LOG(CRIT, LOGGING, ##a)

static struct comm_log saved_comm_log[RTE_MAX_LCORE];

void print_comm_log(uint16_t lcore_id)
{
	struct comm_log *cl = &comm_core_logs[lcore_id];
	struct comm_log *sv = &saved_comm_log[lcore_id];
	struct comm_core_state *ccs = &ccore_state[lcore_id];
	u64 now_real = fp_get_time_ns();
	u64 now_timeslot = (now_real * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
	uint32_t i;

	printf("\ncomm_log lcore %d timeslot 0x%lX (now_timeslot 0x%llX, now - served %lld)",
			lcore_id, ccs->latest_timeslot[0], now_timeslot,
			(s64)(now_timeslot - ccs->latest_timeslot[0]));
	printf("\ntimeslot mul: %d, timeslot shift: %d", TIMESLOT_MUL, TIMESLOT_SHIFT);

#define D(X) (cl->X - sv->X)

#if IS_STRESS_TEST
	double mean_t_btwn_requests_sec, gbps, dropped_gbps, best_gbps;
	mean_t_btwn_requests_sec = cl->mean_t_btwn_requests / rte_get_timer_hz();
	gbps = D(occupied_node_tslots) * 1500 * 8 / (LOG_GAP_SECS * 1000 * 1000 * 1000);
	dropped_gbps = D(dropped_node_tslots) * 1500 * 8 /
			(LOG_GAP_SECS * 1000 * 1000 * 1000);
	best_gbps = cl->stress_test_max_node_tslots * 1500 * 8 /
			(STRESS_TEST_RATE_INCREASE_GAP_SEC * 1000 * 1000 * 1000.0);
	printf("\ncurrent stress test mean t btwn requests: %f, current gbps: %f (dropped: %f), best gbps: %f",
			mean_t_btwn_requests_sec, gbps, dropped_gbps, best_gbps);
	printf("\nstress test rate status (1:increase, 2:maintain, 3:decrease): %lu, increase factor: %f, ",
			cl->stress_test_mode, cl->stress_test_increase_factor);
	printf("ahead: %lu\n", D(stress_test_ahead));
	printf("cores behind:");
	for (i = 0; i < N_ADMISSION_CORES; i++)
		printf(" %lu", cl->core_behind[i]);
#endif

	printf("\n  RX %lu pkts, %lu bytes in %lu batches (%lu non-empty batches), %lu dropped",
			D(rx_pkts), D(rx_bytes), D(rx_batches), D(rx_non_empty_batches),
			D(dropped_rx_due_to_deadline));
	printf("\n  %lu total demand from %lu demand increases %lu demand remained, %lu neg-ack with alloc, %lu demands",
			D(total_demand), D(demand_increased), D(demand_remained), D(neg_acks_with_alloc),
			D(neg_ack_timeslots));
	printf("\n  %lu informative acks for %lu allocations, %lu non-informative",
			D(acks_with_alloc), D(total_acked_timeslots), D(acks_without_alloc));
	printf("\n  processed %lu tslots (%lu non-empty ptn) with %lu node-tslots, diff: %ld",
               D(processed_tslots), D(non_empty_tslots), D(occupied_node_tslots), D(total_demand) - D(occupied_node_tslots));
	printf("\n  TX %lu pkts, %lu bytes, %lu triggers, %lu report-triggers",
			D(tx_pkt), D(tx_bytes), D(triggered_send), D(reports_triggered));
#undef D
	printf("\n");

	printf("\n  RX %lu pkts, %lu bytes in %lu batches (%lu non-empty batches), %lu dropped",
			cl->rx_pkts, cl->rx_bytes, cl->rx_batches, cl->rx_non_empty_batches,
			cl->dropped_rx_due_to_deadline);
	printf("\n  %lu watchdog, %lu non-IPv4, %lu IPv4 non-fastpass",
			cl->rx_watchdog_pkts, cl->rx_non_ipv4_pkts, cl->rx_ipv4_non_fastpss_pkts);
	printf("\n  %lu total demand from %lu demand increases, %lu demand remained",
			cl->total_demand, cl->demand_increased, cl->demand_remained);
	printf("\n  %lu informative acks for %lu allocations, %lu non-informative",
			cl->acks_with_alloc, cl->total_acked_timeslots, cl->acks_without_alloc);
	printf("\n  handled %lu resets", cl->handle_reset);

	printf("\n  processed %lu tslots (%lu non-empty ptn) with %lu node-tslots, diff: %lu",
               cl->processed_tslots, cl->non_empty_tslots, cl->occupied_node_tslots, cl->total_demand - cl->occupied_node_tslots);
	printf("\n  TX %lu pkts (%lu watchdogs), %lu bytes, %lu triggers, %lu report-triggers (%lu due to neg-acks, %lu due to skipped-acks)",
			cl->tx_pkt, cl->tx_watchdog_pkts, cl->tx_bytes, cl->triggered_send, cl->reports_triggered,
			cl->neg_ack_triggered_reports, cl->skipped_ack_triggered_reports);
	printf("\n  set %lu timers, canceled %lu, expired %lu",
			cl->timer_set, cl->timer_cancel, cl->retrans_timer_expired);
	printf("\n  neg acks: %lu without alloc, %lu with alloc with %lu timeslots to %lu dsts",
			cl->neg_acks_without_alloc, cl->neg_acks_with_alloc,
			cl->neg_ack_timeslots, cl->neg_ack_destinations);
	printf("\n  skipped acks: %lu without alloc, %lu with alloc with %lu timeslots",
			cl->skipped_acks_without_alloc, cl->skipped_acks_with_alloc,
			cl->skipped_ack_timeslots);

	printf("\n errors:");
	if (cl->tx_cannot_alloc_mbuf)
		printf("\n  %lu failures to allocate mbuf", cl->tx_cannot_alloc_mbuf);
	if (cl->rx_truncated_pkt)
		printf("\n  %lu rx packets were truncated", cl->rx_truncated_pkt);
	if (cl->areq_invalid_src)
		printf("\n  %lu A-REQ payloads from invalid src (check id map?)", cl->areq_invalid_src);
	if (cl->areq_invalid_dst)
		printf("\n  %lu A-REQ payloads with invalid dst (check id map?)", cl->areq_invalid_dst);
	if (cl->dequeue_admitted_failed)
		printf("\n  %lu times couldn't dequeue a struct admitted_traffic!",
				cl->dequeue_admitted_failed);
	if (cl->error_encoding_packet)
		printf("\n  %lu times couldn't encode packet (due to bug?)",
				cl->error_encoding_packet);
	if (cl->failed_to_allocate_watchdog)
		printf("\n  %lu failed to allocate watchdog packet",
				cl->failed_to_allocate_watchdog);
	if (cl->failed_to_burst_watchdog)
		printf("\n  %lu failed to burst watchdog packet",
				cl->failed_to_burst_watchdog);
	if (cl->admitted_too_many)
		printf("\n  %lu timeslots admitted more than allowed",
				cl->admitted_too_many);

	printf("\n warnings:");
	if (cl->alloc_overflowed_queue)
		printf("\n  %lu alloc overflowed queue", cl->alloc_overflowed_queue);
	if (cl->flush_buffer_in_add_backlog)
		printf("\n  %lu buffer flushes in add backlog (buffer might be too small)",
				cl->flush_buffer_in_add_backlog);
	if (cl->areq_data_count_disagrees)
		printf("\n  %lu A-REQ data counts disagree with cumulative counts (probably due to a lost packet)",
				cl->areq_data_count_disagrees);
	printf("\n");

	memcpy(&saved_comm_log[lcore_id], &comm_core_logs[lcore_id],
			sizeof(struct comm_log));

}

struct admission_statistics saved_admission_statistics;

void print_global_admission_log_parallel_or_pipelined() {
	struct admission_statistics *st = g_admission_stats();
	struct admission_statistics *sv = &saved_admission_statistics;

#if defined(PARALLEL_ALGO)
	printf("\nadmission core (pim with %d ptns, %d nodes per ptn)", N_PARTITIONS, PARTITION_N_NODES);
#else
	printf("\nadmission core (seq with %d algo cores, %d batch size, %d nodes)",
               ALGO_N_CORES, BATCH_SIZE, NUM_NODES);
#endif

#define D(X) (st->X - sv->X)
	printf("\n  enqueue waits: %lu q_head, %lu alloc_new_demands",
			st->wait_for_space_in_q_head, st->new_demands_bin_alloc_failed);
	printf("\n  add_backlog; %lu atomic add %0.2f to avg %0.2f; %lu queue add %0.2f to avg %0.2f",
			st->added_backlog_atomically,
			(float)st->backlog_sum_inc_atomically / (float)(st->added_backlog_atomically+1),
			(float)st->backlog_sum_atomically / (float)(st->added_backlog_atomically+1),
			st->added_backlog_to_queue,
			(float)st->backlog_sum_inc_to_queue / (float)(st->added_backlog_to_queue+1),
			(float)st->backlog_sum_to_queue / (float)(st->added_backlog_to_queue+1));
	printf("\n    %lu bin enqueues (%lu automatic, %lu forced)",
			st->backlog_flush_bin_full + st->backlog_flush_forced,
			st->backlog_flush_bin_full,
			st->backlog_flush_forced);
	printf("\n    %lu spent bins (+%lu), %lu spent demands (+%lu)",
			st->spent_bins, D(spent_bins), st->spent_demands, D(spent_demands));
	printf("\n");
#undef D
}

void save_admission_stats () {
#if (defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO))
	memcpy(&saved_admission_statistics, g_admission_stats(),
			sizeof(saved_admission_statistics));
#elif defined(EMULATION_ALGO)
	emu_save_admission_stats();
#endif
}

void print_global_algo_log() {

#if (defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO))
	print_global_admission_log_parallel_or_pipelined();
#elif defined(EMULATION_ALGO)
	print_global_admission_log_emulation();
#elif defined(BENCHMARK_ALGO)
	print_and_save_global_benchmark_log();
#else
	printf("\nadmission core (UNKNOWN ALGO)");
#endif

	save_admission_stats();
}

struct admission_core_statistics saved_admission_core_statistics[N_ADMISSION_CORES];

void print_admission_core_log_parallel_or_pipelined(uint16_t lcore, uint16_t adm_core_index) {
	int i;
	struct admission_log *al = &admission_core_logs[lcore];
	struct admission_core_statistics *st = g_admission_core_stats(adm_core_index);
	struct admission_core_statistics *sv = &saved_admission_core_statistics[adm_core_index];

#define D(X) (st->X - sv->X)
	printf("admission lcore %d: %lu no_timeslot, %lu need more (avg %0.2f), %lu done",
			lcore, st->no_available_timeslots_for_bin_entry,
			st->allocated_backlog_remaining,
			(float)st->backlog_sum / (float)(st->allocated_backlog_remaining+1),
			st->allocated_no_backlog);
	printf("\n  %lu fail_alloc_admitted, %lu q_admitted_full. %lu bin_alloc_fail, %lu q_out_full, %lu q_spent_full, %lu wait_token",
			st->admitted_traffic_alloc_failed, st->wait_for_space_in_q_admitted_out,
			st->out_bin_alloc_failed, st->wait_for_space_in_q_bin_out,
			st->wait_for_space_in_q_spent, st->waiting_to_pass_token);
	printf("\n  %lu flushed q_out (%lu automatic, %lu forced); %lu flushed q_spent (%lu automatic, %lu forced); processed from q_head %lu bins, %lu demands; run_passed %lu bins; wrap up passed %lu bins, internal %lu bins %lu demands",
			st->q_out_flush_bin_full + st->q_out_flush_batch_finished,
			st->q_out_flush_bin_full, st->q_out_flush_batch_finished,
			st->q_spent_flush_bin_full + st->q_spent_flush_batch_finished,
			st->q_spent_flush_bin_full, st->q_spent_flush_batch_finished,
			st->new_request_bins, st->new_requests,
			st->passed_bins_during_run,
			st->passed_bins_during_wrap_up,
				st->wrap_up_non_empty_bin, st->wrap_up_non_empty_bin_demands);

#if defined(PARALLEL_ALGO)
	printf("\n    %lu phases completed, %lu not ready, %lu out of order",
               st->phase_finished, st->phase_none_ready, st->phase_out_of_order);
#endif

	printf("\n");
#undef D

	printf("  backlog_hist: ");
	for (i = 0; i < BACKLOG_HISTOGRAM_NUM_BINS; i++)
		printf("%lu ", st->backlog_histogram[i]);
	printf ("\n");

#if defined(PIPELINED_ALGO)
	printf("  bin_size >> %d: ", BIN_SIZE_HISTOGRAM_SHIFT);
	for (i = 0; i < BIN_SIZE_HISTOGRAM_NUM_BINS; i++)
		printf("%lu ", st->bin_size_histogram[i]);
	printf ("\n");

	if (MAINTAIN_CORE_BIN_HISTOGRAM) {
		printf("  #demands in internal_bin >> %d: ", CORE_BIN_HISTOGRAM_SHIFT);
		for (i = 0; i < CORE_BIN_HISTOGRAM_NUM_BINS; i++)
			printf("%lu ", st->core_bins_histogram[i]);
		printf ("\n");
	}

	if (MAINTAIN_AFTER_TSLOTS_HISTOGRAM) {
		printf("  after_tslots >> %d: ", AFTER_TSLOTS_HISTOGRAM_SHIFT);
		for (i = 0; i < AFTER_TSLOTS_HISTOGRAM_NUM_BINS; i++)
			printf("%lu ", al->after_tslots_histogram[i]);
		printf ("\n");
	}
#endif
}

static struct admission_log saved_admission_logs[RTE_MAX_LCORE];

void print_admission_core_log_emulation(uint16_t lcore, uint16_t adm_core_index) {
	struct admission_log *al = &admission_core_logs[lcore];
	struct admission_log *sal = &saved_admission_logs[lcore];

#define D(X) (al->X - sal->X)
	printf("\nadmission lcore %d, timeslot %lu, tslot diff %lu\n", lcore,
			al->current_timeslot, D(current_timeslot));
	printf("  tslots core ahead: %lu (diff), %lu (total)\n", D(core_ahead),
			al->core_ahead);
#undef D

	print_core_admission_log_emulation(adm_core_index);

	/* save the admission logs for this core */
	memcpy(sal, al, sizeof(saved_admission_logs[lcore]));
}

void save_admission_core_stats(int i) {
#if (defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO))
	memcpy(&saved_admission_core_statistics[i],
	       g_admission_core_stats(i),
			sizeof(saved_admission_core_statistics[i]));
#else
	emu_save_admission_core_stats(i);
#endif
}

void print_core_log(uint16_t lcore, uint16_t adm_core_index) {

#if (defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO))
	print_admission_core_log_parallel_or_pipelined(lcore, adm_core_index);
#elif (defined(EMULATION_ALGO))
	print_admission_core_log_emulation(lcore, adm_core_index);
#else
	print_and_save_benchmark_core_log(adm_core_index);
#endif

}

LogCore::LogCore(uint64_t log_gap_ticks, uint64_t q_log_gap_ticks)
	: m_log_gap_ticks(log_gap_ticks), m_q_log_gap_ticks(q_log_gap_ticks)
{}

void LogCore::add_comm_lcore(uint8_t lcore)
{
	m_comm_lcores.push_back(lcore);
}

void LogCore::add_logged_lcore(uint8_t lcore)
{
	m_logged_lcores.push_back(lcore);
}

void LogCore::add_queueing_stats(struct queue_bank_stats* queue_stats)
{
	m_queue_stats.push_back(queue_stats);
}

void LogCore::add_drop_stats(struct port_drop_stats *port_stats)
{
	m_port_stats.push_back(port_stats);
}

int LogCore::exec()
{
	uint64_t next_ticks = rte_get_timer_cycles();
	uint64_t q_next_ticks = rte_get_timer_cycles();
	int i, j;
	struct conn_log_struct conn_log;
	FILE *fp;
	FILE *fp_queues;
	char filename[MAX_FILENAME_LEN];
	char filename_queues[MAX_FILENAME_LEN];
	u64 time = fp_get_time_ns();
	u64 time_prev;

	snprintf(filename, MAX_FILENAME_LEN, "log/conn-%016llX.csv",
			time);
	snprintf(filename_queues, MAX_FILENAME_LEN, "log/queues-%016llX.csv",
			time);

	/* open file for conn log */
	fp = fopen(filename, "w");
	if (fp == NULL) {
		LOGGING_ERR("lcore %d could not open file for logging: %s\n",
				rte_lcore_id(), filename);
		return -1;
	}

	/* open file for queues log */
	fp_queues = fopen(filename_queues, "w");
	if (fp_queues == NULL) {
		LOGGING_ERR("lcore %d could not open file for queue logging: %s\n",
				rte_lcore_id(), filename_queues);
		return -1;
	}

	/* copy baseline statistics */
	for (i = 0; i < m_comm_lcores.size(); i++)
		memcpy(&saved_comm_log[i], &comm_core_logs[m_comm_lcores[i]],
				sizeof(struct comm_log));
	save_admission_stats();
	for (i = 0; i < N_ADMISSION_CORES; i++)
		save_admission_core_stats(i);

	while (1) {
		/* wait until proper time for main log */
		while (next_ticks > rte_get_timer_cycles()) {
#if (defined(EMULATION_ALGO) && MAINTAIN_QUEUE_BANK_LOG_COUNTERS)
			/* while waiting, sample queues and write to file */
			while (q_next_ticks > rte_get_timer_cycles())
				rte_pause();

			/* print time now */
			time = fp_get_time_ns();

			for (i = 0; i < m_queue_stats.size(); i++) {
				fprintf(fp_queues, "router number %d\n", i);
				print_queue_bank_stats_to_file(fp_queues, m_queue_stats[i],
						time);
			}
			for (i = 0; i < m_port_stats.size(); i++) {
				fprintf(fp_queues, "core number %d\n", i);
				print_port_drop_stats_to_file(fp_queues, m_port_stats[i],
						time);
			}
			q_next_ticks += m_q_log_gap_ticks;
#else
			rte_pause();
#endif
		}

		for (i = 0; i < m_comm_lcores.size(); i++)
			print_comm_log(m_comm_lcores[i]);
		print_global_algo_log();

		for (i = 0; i < m_logged_lcores.size(); i++)
			print_core_log(m_logged_lcores[i], i);
		fflush(stdout);

		/* save admission core stats */
		for (i = 0; i < N_ADMISSION_CORES; i++)
			save_admission_core_stats(i);

		/* write log */
//		for (i = 0; i < MAX_NODES; i++) {
		for (i = 49; i < 55; i++) {
			conn_log.version = CONN_LOG_STRUCT_VERSION;
			conn_log.node_id = i;
			conn_log.timestamp = fp_get_time_ns();
			comm_dump_stat(i, &conn_log);

#if (defined(PIPELINED_ALGO) || defined(PARALLEL_ALGO))
			/* get backlog */
			conn_log.backlog = 0;
			for (j = 0; j < MAX_NODES; j++)
				conn_log.backlog +=
					backlog_get(g_admission_backlog(), i, j);
#endif

			if (fwrite(&conn_log, sizeof(conn_log), 1, fp) != 1)
				LOGGING_ERR("couldn't write conn info of node %d to file\n", i);
		}

		fflush(fp);
		fflush(fp_queues);

		next_ticks += m_log_gap_ticks;
	}

	return 0;
}

static int exec_log_core(void *log_core)
{
	return ((LogCore *)log_core)->exec();
}

void LogCore::remote_launch(unsigned lcore)
{
	rte_eal_remote_launch(exec_log_core, this, lcore);
}
