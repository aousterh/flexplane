/*
 * emu_comm_core_map.h
 *
 *  Created on: November 9, 2015
 *      Author: aousterh
 */

#ifndef EMU_COMM_CORE_MAP_H_
#define EMU_COMM_CORE_MAP_H_

#include "../arbiter/control.h"
#include "emu_topology.h"
#include <inttypes.h>

/* The number of cores for the comm_index-th comm core/stress test core. */
static inline uint16_t cores_for_comm(uint16_t comm_index)
{
	uint16_t base_cores_per_comm = ALGO_N_CORES / N_COMM_CORES;

	if (comm_index < ALGO_N_CORES % N_COMM_CORES)
		return base_cores_per_comm + 1;
	else
		return base_cores_per_comm;
}

/* The number of endpoints for the comm_index-th comm core/stress test core. */
static inline uint16_t endpoints_for_comm(uint16_t comm_index,
		struct emu_topo_config *topo_config)
{
	uint16_t cores_per_comm = ALGO_N_CORES / N_COMM_CORES;
	uint16_t cumulative_cores, n_epgs;

	/* Cores are distributed evenly across the comm cores. */
	if (comm_index < ALGO_N_CORES % N_COMM_CORES) {
		cores_per_comm++;
		cumulative_cores = cores_per_comm * comm_index;
	} else {
		cumulative_cores = (cores_per_comm + 1) *
				(ALGO_N_CORES % N_COMM_CORES);
		cumulative_cores += cores_per_comm *
				(comm_index - (ALGO_N_CORES % N_COMM_CORES));
	}

	/* The first num_endpoint_groups(topo_config) cores include an endpoint
	 * group each. */
	if (num_endpoint_groups(topo_config) > cumulative_cores)
		n_epgs = cores_per_comm;
	else if (num_endpoint_groups(topo_config) <=
			cumulative_cores - cores_per_comm)
		n_epgs = 0;
	else
		n_epgs = num_endpoint_groups(topo_config) -
				(cumulative_cores - cores_per_comm);

	return n_epgs * endpoints_per_epg(topo_config);
}

#endif /* EMU_COMM_CORE_MAP_H_ */
