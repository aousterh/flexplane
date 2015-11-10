/*
 * admission_core.h
 *
 *  Created on: May 18, 2014
 *      Author: aousterh
 */

#ifndef ADMISSION_CORE_H
#define ADMISSION_CORE_H

#include <rte_ring.h>
#include "../graph-algo/algo_config.h"


#ifdef PARALLEL_ALGO
/* parallel algo, e.g. pim */
#include "pim_admission_core.h"
#include "../grant-accept/pim.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline
void admission_init_global(struct rte_ring **q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool,
		struct emu_topo_config *topo_config)
{
	pim_admission_init_global(q_admitted_out[0], admitted_traffic_mempool);
}

static inline
void admission_init_core(uint16_t lcore_id) {
	pim_admission_init_core(lcore_id);
}

static inline
int exec_admission_core(void *void_cmd_p) {
	exec_pim_admission_core(void_cmd_p);
}

static inline
struct admissible_state *g_admissible_status(void) {
	return (struct admissible_state *) &g_pim_state;
}

static inline
struct backlog *g_admission_backlog(void) {
	return &g_pim_state.backlog;
}

static inline
struct admission_core_statistics *g_admission_core_stats(uint16_t i) {
	return &g_pim_state.cores[i].stat;
}

static inline
struct admission_statistics *g_admission_stats(void) {
	return &g_pim_state.stat;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

#ifdef PIPELINED_ALGO
/* pipelined algo */
#include "seq_admission_core.h"
#include "../graph-algo/admissible_structures.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline
void admission_init_global(struct rte_ring **q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool,
		struct emu_topo_config *topo_config)
{
	seq_admission_init_global(q_admitted_out[0], admitted_traffic_mempool);
}

static inline
void admission_init_core(uint16_t lcore_id) {
	seq_admission_init_core(lcore_id);
}

static inline
int exec_admission_core(void *void_cmd_p) {
	exec_seq_admission_core(void_cmd_p);
}

static inline
struct admissible_state *g_admissible_status(void) {
	return (struct admissible_state *) &g_seq_admissible_status;
}

static inline
struct backlog *g_admission_backlog(void) {
	return &g_seq_admissible_status.backlog;
}

static inline
struct admission_core_statistics *g_admission_core_stats(uint16_t i) {
	return &g_seq_admissible_status.cores[i].stat;
}

static inline
struct admission_statistics *g_admission_stats(void) {
	return &g_seq_admissible_status.stat;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

#ifdef EMULATION_ALGO
/* emulation algo */
#include "emu_admission_core.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline
void admission_init_global(struct rte_ring **q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool,
		struct emu_topo_config *topo_config)
{
	emu_admission_init_global(q_admitted_out, admitted_traffic_mempool,
			topo_config);
}

static inline
void admission_init_core(uint16_t lcore_id) {
        /* do nothing */
}

static inline
int exec_admission_core(void *void_cmd_p) {
	exec_emu_admission_core(void_cmd_p);
}

static inline
struct admissible_state *g_admissible_status(void) {
	return (struct admissible_state *) emu_get_instance();
}

static inline
struct admission_core_statistics *g_admission_core_stats(uint16_t i) {
	return NULL; /* unused */
}

static inline
struct admission_statistics *g_admission_stats(void) {
	return NULL; /* unused */
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

#ifdef BENCHMARK_ALGO
#include "benchmark.h"

static inline
void admission_init_global(struct rte_ring *q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool,
		struct emu_topo_config *topo_config)
{
	/* do nothing */
}

static inline
void admission_init_core(uint16_t lcore_id) {
	/* do nothing */
}

static inline
int exec_admission_core(void *void_cmd_p) {
	/* do nothing */
}

static inline
struct admissible_state *g_admissible_status(void) {
	return (struct admissible_state *) benchmark_get_instance();
}

static inline
struct admission_core_statistics *g_admission_core_stats(uint16_t i) {
	return NULL; /* unused */
}

static inline
struct admission_statistics *g_admission_stats(void) {
	return NULL; /* unused */
}

#endif


#endif /* ADMISSION_CORE_H */
