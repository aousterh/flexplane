/*
 * emu_admission_core.h
 *
 *  Created on: July 12, 2014
 *      Author: aousterh
 */

#ifndef EMU_ADMISSION_CORE_H
#define EMU_ADMISSION_CORE_H

#include <rte_ring.h>

#define		ADMITTED_TRAFFIC_MEMPOOL_SIZE		(16*1024)
#define		ADMITTED_TRAFFIC_CACHE_SIZE			512

/* emu state */
struct Emulation;
struct queue_bank_stats;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct Emulation *emu_get_instance(void);

struct queue_bank_stats *emu_get_queueing_stats(uint8_t router_index);
struct port_drop_stats *emu_get_port_stats(uint8_t router_index);
uint16_t emu_get_num_routers();

void emu_admission_init_global(struct rte_ring **q_admitted_out,
		struct rte_mempool *admitted_traffic_mempool);

/**
 * Runs the admission core
 */
int exec_emu_admission_core(void *void_cmd_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EMU_ADMISSION_CORE_H */
