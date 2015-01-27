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

#include "api.h"
#include "../protocol/platform/generic.h"

#define		MAINTAIN_EMU_ADM_LOG_COUNTERS	1

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

	/* framework failures */
	uint64_t send_packet_failed;
	uint64_t admitted_struct_overflow;

	/* counters used by algos */
	uint64_t endpoint_sent_packet;
	uint64_t router_sent_packet;
	uint64_t endpoint_dropped_packet;
	uint64_t router_dropped_packet;
	uint64_t router_marked_packet;
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
void adm_log_emu_send_packets_failed(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->send_packet_failed += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_admitted_struct_overflow(
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->admitted_struct_overflow++;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_sent_packets(
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_sent_packet += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_router_sent_packets (
		struct emu_admission_core_statistics *st, uint32_t n_pkts) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->router_sent_packet += n_pkts;
}

static inline __attribute__((always_inline))
void adm_log_emu_endpoint_dropped_packet (
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->endpoint_dropped_packet++;
}

static inline __attribute__((always_inline))
void adm_log_emu_router_dropped_packet (
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->router_dropped_packet++;
}

static inline __attribute__((always_inline))
void adm_log_emu_router_marked_packet (
		struct emu_admission_core_statistics *st) {
	if (MAINTAIN_EMU_ADM_LOG_COUNTERS)
		st->router_marked_packet++;
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
