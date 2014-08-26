/*
 * Defines relating to the algorithm being used
 */

#ifndef ALGO_CONFIG_H_
#define ALGO_CONFIG_H_

#if ((defined(PARALLEL_ALGO) && defined(PIPELINED_ALGO)) || \
     (defined(PARALLEL_ALGO) && defined(EMULATION_ALGO)) ||  \
     (defined(EMULATION_ALGO) && defined(PIPELINED_ALGO)))
#error "Multiple ALGOs are defined"
#endif
#if !(defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO) || defined(EMULATION_ALGO))
#error "No ALGO is defined"
#endif

#ifdef PARALLEL_ALGO
/* parallel algo */
#include "../grant-accept/partitioning.h"

#ifndef ALGO_N_CORES
#define ALGO_N_CORES			        N_PARTITIONS
#endif

#endif

#ifdef PIPELINED_ALGO
/* pipelined algo */

#ifndef ALGO_N_CORES
#define ALGO_N_CORES				4
#endif

#endif

#ifdef EMULATION_ALGO
/* emulation */

#ifndef ALGO_N_CORES
#define ALGO_N_CORES				1
#endif

#endif

#endif /* ALGO_CONFIG_H_ */
