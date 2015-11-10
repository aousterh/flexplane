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

	/* When the number of ALGO cores is not evenly divisible by the number of
	 * comm cores, assign more ALGO cores to the higher numbered comm cores
	 * because the higher numbered ALGO cores might be Core routers (which are
	 * less work) */
	if (comm_index >= N_COMM_CORES - ALGO_N_CORES % N_COMM_CORES)
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

	/* Emu cores are distributed evenly across the comm cores. */
	if (comm_index >= N_COMM_CORES - ALGO_N_CORES % N_COMM_CORES) {
		cumulative_cores = (cores_per_comm) *
				(N_COMM_CORES - ALGO_N_CORES % N_COMM_CORES);
		cumulative_cores += (cores_per_comm + 1) *
				(comm_index + 1 - (N_COMM_CORES - ALGO_N_CORES % N_COMM_CORES));
		cores_per_comm++;
	} else {
		cumulative_cores = cores_per_comm * (comm_index + 1);
	}

	/* The first num_endpoint_groups(topo_config) cores include an endpoint
	 * group each. */
	if (num_endpoint_groups(topo_config) > cumulative_cores)
		n_epgs = cores_per_comm;
	else if (num_endpoint_groups(topo_config) <=
			cumulative_cores - cores_per_comm)
		n_epgs = 0;
	else {
		n_epgs = num_endpoint_groups(topo_config) -
				(cumulative_cores - cores_per_comm);
	}

	return n_epgs * endpoints_per_epg(topo_config);
}

#endif /* EMU_COMM_CORE_MAP_H_ */
