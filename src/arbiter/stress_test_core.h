
#ifndef STRESS_TEST_CORE_H_
#define STRESS_TEST_CORE_H_

#include <stdint.h>
#include <rte_ip.h>
#include "../graph-algo/admissible.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_ENQUEUES_PER_LOOP		1024
#define STRESS_TEST_MAX_ADMITTED_PER_LOOP		(32*BATCH_SIZE)

/* The buffer size when writing to q_head */
#define Q_HEAD_WRITE_BUFFER_SIZE		(32*1024)

/**
 * Specifications for controller thread
 */
struct stress_test_core_cmd {
	uint64_t start_time;
	uint64_t end_time;
	uint64_t first_time_slot;

	double mean_t_btwn_requests;
	uint32_t num_nodes;
	uint32_t first_node;
	uint32_t demand_tslots;

	uint32_t num_initial_srcs;
	uint32_t num_initial_dsts_per_src;
	uint32_t initial_flow_size;

	struct rte_ring **q_allocated;
	uint32_t num_q_allocated;
	struct rte_mempool *admitted_traffic_mempool;
};

/**
 * If there are multiple stress test cores, n-1 of them will be slaves.
 */
int exec_slave_stress_test_core(void *void_cmd_p);
int exec_stress_test_core(void *void_cmd_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STRESS_TEST_CORE_H_ */
