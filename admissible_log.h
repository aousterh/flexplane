/*
 * admissible_log.h
 *
 * Logging for admission control in the emulation arbiter.
 *
 *  Created on: September 5, 2014
 *      Author: aousterh
 */

#ifndef EMU_ADMISSIBLE_LOG_H__
#define EMU_ADMISSIBLE_LOG_H__

#include <stdint.h>

#define		MAINTAIN_EMU_ADM_LOG_COUNTERS	0

/**
 * Per-core statistics for emulation
 */
struct emu_admission_core_statistics {
	/* admitted structs */
	uint64_t wait_for_admitted_enqueue;
	uint64_t admitted_alloc_failed;

	/* packets */
	uint64_t admitted_packet;
	uint64_t dropped_packet;
	uint64_t marked_packet;
	uint64_t admitted_mtu;

	/* framework failures */
	uint64_t send_packet_failed;

	/* counters used by drivers */
	uint64_t endpoint_driver_processed_new;
	uint64_t endpoint_driver_pulled;
	uint64_t endpoint_driver_pushed;
	uint64_t router_driver_pulled;
	uint64_t router_driver_pushed;

	uint64_t router_driver_step_end;
	uint64_t router_driver_step_begin;
	uint64_t endpoint_driver_push_begin;
	uint64_t endpoint_driver_pull_begin;
	uint64_t endpoint_driver_new_begin;
};

/**
 * Global statistics for emulation
 */
struct emu_admission_statistics {
	/* framework failures */
	uint64_t packet_alloc_failed;
	uint64_t enqueue_backlog_failed;
	uint64_t enqueue_reset_failed;
};

/*
 * Copy the global admission statistics to use to compute the difference
 * over the logging time interval.
 */
void emu_save_admission_stats();

/*
 * Copy the per-core admission statistics to use to compute the difference
 * over the logging time interval.
 */
void emu_save_admission_core_stats(int i);

/* per-core admission stats */

static inline __attribute__((always_inline))
void adm_log_emu_wait_for_admitted_enqueue(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->wait_for_admitted_enqueue++;
}

static inline __attribute__((always_inline))
void adm_log_emu_admitted_alloc_failed(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->admitted_alloc_failed++;
}

static inline __attribute__((always_inline))
void adm_log_emu_admitted_packet(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->admitted_packet++;
}

static inline __attribute__((always_inline))
void adm_log_emu_dropped_packet(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->dropped_packet++;
}

static inline __attribute__((always_inline))
void adm_log_emu_marked_packet(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->marked_packet++;
}

static inline __attribute__((always_inline))
void adm_log_emu_admitted_mtus(
		struct emu_admission_core_statistics *st, uint32_t n_mtus) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->admitted_mtu += n_mtus;
}

static inline __attribute__((always_inline))
void adm_log_emu_send_packets_failed(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->send_packet_failed += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_processed_new(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_processed_new += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_pulled(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_pulled += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_pushed(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_pushed += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_router_driver_pulled(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->router_driver_pulled += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_router_driver_pushed(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS) {
		st->router_driver_pushed += n_pkts;
		st->router_driver_step_end++;
	}
}

static inline __attribute__((always_inline))
void adm_log_emu_router_driver_step_begin(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->router_driver_step_begin++;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_push_begin(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_push_begin++;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_pull_begin(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_pull_begin++;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_driver_new_begin(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_driver_new_begin++;
}

/* global admission stats */

static inline __attribute__((always_inline))
void adm_log_emu_packet_alloc_failed(
		struct emu_admission_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->packet_alloc_failed++;
}

static inline __attribute__((always_inline))
void adm_log_emu_enqueue_backlog_failed(
		struct emu_admission_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->enqueue_backlog_failed += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_enqueue_reset_failed(
		struct emu_admission_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->enqueue_reset_failed++;
}

#endif /* EMU_ADMISSIBLE_LOG_H__ */
