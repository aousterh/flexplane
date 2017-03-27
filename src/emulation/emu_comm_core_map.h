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

/* The number of comm cores that have the smaller number of emu cores. Note
 * that all comm cores have either ALGO_N_CORES / N_COMM_CORES emu cores, or
 * one more than that. */
static inline uint16_t comms_with_min_emu_cores()
{
	return N_COMM_CORES - ALGO_N_CORES % N_COMM_CORES;
}

/* The minimum number of emu cores assigned to each comm core (all comm cores
 * have this many or this many plus 1 emu cores). */
static inline uint16_t min_emu_cores_per_comm()
{
	return ALGO_N_CORES / N_COMM_CORES;
}

/* The number of cores for the comm_index-th comm core/stress test core. */
static inline uint16_t cores_for_comm(uint16_t comm_index)
{
	/* When the number of ALGO cores is not evenly divisible by the number of
	 * comm cores, assign more ALGO cores to the higher numbered comm cores
	 * because the higher numbered ALGO cores might be Core routers (which are
	 * less work) */
	if (comm_index >= comms_with_min_emu_cores())
		return min_emu_cores_per_comm() + 1;
	else
		return min_emu_cores_per_comm();
}

/* The number of endpoints for the comm_index-th comm core/stress test core. */
static inline uint16_t endpoints_for_comm(uint16_t comm_index,
		struct emu_topo_config *topo_config)
{
	uint16_t cores_per_comm = min_emu_cores_per_comm();
	uint16_t cumulative_cores, n_epgs;

	/* Emu cores are distributed evenly across the comm cores. */
	if (comm_index >= comms_with_min_emu_cores()) {
		cumulative_cores = (cores_per_comm) * comms_with_min_emu_cores();
		cumulative_cores += (cores_per_comm + 1) *
				(comm_index + 1 - comms_with_min_emu_cores());
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

/* The index of the comm core for emu core with index emu_index. */
static inline uint16_t comm_for_emu(uint16_t emu_index)
{
	uint16_t index_in_comms_w_more_cores;

	if (emu_index < min_emu_cores_per_comm() * comms_with_min_emu_cores()) {
		/* this core is assigned to a comm with fewer emu cores */
		return emu_index / min_emu_cores_per_comm();
	} else {
		/* this core is assigned to a comm with more emu cores */
		index_in_comms_w_more_cores = (emu_index - min_emu_cores_per_comm() *
				comms_with_min_emu_cores()) / (min_emu_cores_per_comm() + 1);
		return comms_with_min_emu_cores() + index_in_comms_w_more_cores;
	}
}

#endif /* EMU_COMM_CORE_MAP_H_ */
