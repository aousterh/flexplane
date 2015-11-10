
#include "control.h"

#include <rte_cycles.h>
#include <rte_errno.h>
#include "port_alloc.h"
#include "main.h"
#include "comm_core.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "benchmark_core.h"
#include "path_sel_core.h"
#include "log_core.h"
#include "stress_test_core.h"
#include "../emulation/emu_topology.h"
#include "../emulation/emu_comm_core_map.h"

int control_do_queue_allocation(void)
{
	int ret, i, j;

	/* If we don't need network, return */
	if (!(EXPT_RUN_MASK)) {
		return 0;
	}

	if(n_enabled_lcore < N_ADMISSION_CORES + N_COMM_CORES + N_LOG_CORES +
			N_PATH_SEL_CORES + N_BENCHMARK_CORES) {
		rte_exit(EXIT_FAILURE,
				"Need #alloc + #comm + #log + #path_sel cores (need %d, got %d)\n",
				N_ADMISSION_CORES + N_COMM_CORES + N_LOG_CORES +
				N_PATH_SEL_CORES + N_BENCHMARK_CORES, n_enabled_lcore);
	}

	if(n_enabled_port < N_CONTROLLER_PORTS) {
		rte_exit(EXIT_FAILURE, "Need %d enabled ports, got %d\n",
				N_CONTROLLER_PORTS, n_enabled_port);
	}

	/** TX queues */
	for (i = 0; i < N_COMM_CORES; i++) {
		for (j = 0; j < n_enabled_port; j++) {
			ret = conf_alloc_tx_queue(enabled_lcore[i], enabled_port[j]);
			if (ret != 0)
				return ret;
		}
	}

	/** RX queues */
	for (i = 0; i < N_CONTROLLER_PORTS; i++) {
		/* First half of RX ports go to the controller, enabled lcore 0 */
		ret = conf_alloc_rx_queue(enabled_lcore[0], enabled_port[i]);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

void launch_comm_cores(uint64_t start_time, uint64_t end_time,
		uint64_t first_time_slot, struct rte_ring **q_path_selected,
		struct rte_ring **q_admitted,
		struct rte_mempool *admitted_traffic_mempool)
{
	struct comm_core_cmd comm_cmd;
	unsigned lcore_id = rte_lcore_id();
	unsigned socket = rte_lcore_to_socket_id(lcore_id);

	if (lcore_conf[lcore_id].n_rx_queue != 1)
		rte_exit(EXIT_FAILURE,
				"comm core supports 1 queue, %d were configured\n",
				lcore_conf[rte_lcore_id()].n_rx_queue);

	// Set commands
	comm_cmd.start_time = start_time;
	comm_cmd.end_time = end_time;
	comm_cmd.q_allocated =
			((N_PATH_SEL_CORES > 0) ? q_path_selected : q_admitted);
	comm_cmd.admitted_traffic_mempool = admitted_traffic_mempool;
	comm_cmd.rx_queue_id = lcore_conf[lcore_id].rx_queue_list[0].queue_id;
	comm_cmd.tx_queue_id = lcore_conf[lcore_id].enabled_ind;
	comm_cmd.port_id = lcore_conf[lcore_id].rx_queue_list[0].port_id;
	rte_eth_macaddr_get(comm_cmd.port_id, &comm_cmd.eth_addr);
	comm_cmd.tx_pktmbuf_pool = tx_pktmbuf_pool[socket];

	/* initialize comm core on this core */
	comm_init_core(rte_lcore_id(), first_time_slot);

	/** Run the controller on this core */
	exec_comm_core(&comm_cmd);
}

void launch_stress_test_cores(uint64_t start_time,
		uint64_t end_time, uint64_t first_time_slot,
		struct rte_ring **q_path_selected,
		struct rte_ring **q_admitted,
		struct rte_mempool *admitted_traffic_mempool,
		struct emu_topo_config *topo_config)
{
	struct stress_test_core_cmd cmd[N_COMM_CORES];
	uint64_t hz = rte_get_timer_hz();
	uint16_t i;

	/* Prepare commands */
	uint32_t node_index = 0;
	uint32_t q_admitted_index = 0;
	for (i = 0; i < N_COMM_CORES; i++) {
		cmd[i].start_time = start_time;
		cmd[i].end_time = start_time + hz * STRESS_TEST_DURATION_SEC;
		cmd[i].first_time_slot = first_time_slot;
		cmd[i].mean_t_btwn_requests = STRESS_TEST_MEAN_T_BETWEEN_REQUESTS_SEC * hz;
		cmd[i].demand_tslots = STRESS_TEST_DEMAND_TSLOTS;
		cmd[i].num_initial_srcs = STRESS_TEST_INITIAL_SOURCES;
		cmd[i].num_initial_dsts_per_src = STRESS_TEST_INITIAL_DSTS_PER_SRC;
		cmd[i].initial_flow_size = STRESS_TEST_INITIAL_FLOW_SIZE;
		cmd[i].admitted_traffic_mempool = admitted_traffic_mempool;
#ifdef EMULATION_ALGO
		cmd[i].num_nodes = endpoints_for_comm(i, topo_config);
		cmd[i].q_allocated = &q_admitted[q_admitted_index];
		cmd[i].num_q_allocated = cores_for_comm(i);
		q_admitted_index += cmd[i].num_q_allocated;
#else
		cmd[i].num_nodes = remaining_nodes / (N_COMM_CORES - i);
		cmd[i].q_allocated =
					((N_PATH_SEL_CORES > 0) ? q_path_selected : q_admitted);
#endif
		cmd[i].first_node = node_index;
		node_index += cmd[i].num_nodes;
		printf("STRESS TEST CORE %d: %d nodes (first: %d), %d admitted qs\n",
				i, cmd[i].num_nodes, cmd[i].first_node,
				cmd[i].num_q_allocated);
	}

	/* launch all but last stress test core */
	for (i = 1; i < N_COMM_CORES; i++) {
		uint16_t lcore_id = enabled_lcore[FIRST_COMM_CORE + i];
		rte_eal_remote_launch(exec_slave_stress_test_core, &cmd[i], lcore_id);
	}

	/** Run the last stress test core on this core */
	exec_stress_test_core(&cmd[0]);
}

struct rte_mempool *allocate_admitted_traffic_mempool(int socketid)
{
	struct rte_mempool *pool;
	int elem_size;

#if (defined(EMULATION_ALGO) || defined(BENCHMARK_ALGO))
	elem_size = sizeof(struct emu_admitted_traffic);
#else
	elem_size = sizeof(struct admitted_traffic);
#endif

	pool =
		rte_mempool_create("admitted_traffic_pool",
			ADMITTED_TRAFFIC_MEMPOOL_SIZE, /* num elements */
			elem_size, /* element size */
			ADMITTED_TRAFFIC_CACHE_SIZE, /* cache size */
			0, NULL, NULL, NULL, NULL, /* custom initialization, disabled */
			socketid, 0);
	if (pool == NULL)
		rte_exit(EXIT_FAILURE,
				"Cannot init admitted traffic pool on socket %d: %s\n", socketid,
				rte_strerror(rte_errno));

	CONTROL_INFO("Allocated admitted traffic pool on socket %d - %lu bufs\n",
			socketid, (uint64_t)ADMITTED_TRAFFIC_MEMPOOL_SIZE);

	return pool;
}


/**
 * Enqueues commands for allocation network experiments
 *
 * @return number of remaining experiments after this one
 */
void launch_cores(void)
{
	/* variables */
	//struct comm_core_cmd comm_cores[RTE_MAX_LCORE];
	uint64_t start_time;
	static uint64_t end_time;
	int i; (void)i;
	struct admission_core_cmd admission_cmd[N_ADMISSION_CORES];
	struct path_sel_core_cmd path_sel_cmd;
	struct benchmark_core_cmd bench_cmd[N_BENCHMARK_CORES];
	uint64_t first_time_slot;
	uint64_t now;
	struct rte_ring *q_admitted[MAX_Q_ADMITTED];
	struct rte_ring *q_path_selected;
	struct rte_mempool *admitted_traffic_mempool;
	LogCore *log_core;
	Benchmark *benchmark;
	char s[64];
	uint16_t n_q_admitted = 1;
	struct emu_topo_config topo_config;

	benchmark_cost_of_get_time();

	/* decide what the first time slot to be output is */
	now = fp_get_time_ns();
	first_time_slot = ((now + INIT_MAX_TIME_NS) * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
	CONTROL_INFO("now %lu first time slot will be %lu\n", now, first_time_slot);

	/*** LOGGING OUTPUT ***/
#ifdef LOG_TO_STDOUT
	rte_openlog_stream(stdout);
#endif

	/*** GLOBAL INIT ***/
	/* initialize comm core global data */
	comm_init_global_structs(first_time_slot);

#ifdef EMULATION_ALGO
	/* initialize emulated network topology */
	topo_config.num_racks = EMU_NUM_RACKS;
	topo_config.rack_shift = 5;
	if (SEPARATE_RACKS == 1)
		topo_config.num_core_rtrs = 0;
	else
		topo_config.num_core_rtrs = 1;

	/* create a q_admitted_out for each algo core */
	if (ALGO_N_CORES > MAX_Q_ADMITTED)
		rte_exit(EXIT_FAILURE, "ALGO_N_CORES exceeds max q admitted\n");
	n_q_admitted = ALGO_N_CORES;
#endif
	/* create admitted_out queues */
	for (i = 0; i < n_q_admitted; i++) {
		snprintf(s, sizeof(s), "q_admitted_%d", i);
		q_admitted[i] = rte_ring_create(s, ADMITTED_TRAFFIC_MEMPOOL_SIZE,
				0, RING_F_SP_ENQ | RING_F_SC_DEQ);
		if (q_admitted[i] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot init q_admitted: %s\n",
					rte_strerror(rte_errno));
	}

	/* create q_path_selected_out */
	q_path_selected = rte_ring_create("q_path_selected",
			2 * ADMITTED_TRAFFIC_MEMPOOL_SIZE, 0, 0);
	if (q_path_selected == NULL)
		rte_exit(EXIT_FAILURE,
				"Cannot init q_path_selected: %s\n", rte_strerror(rte_errno));

	/* create admitted_traffic_mempool */
	admitted_traffic_mempool = allocate_admitted_traffic_mempool(0);

	/* initialize admission core global data */
	admission_init_global(&q_admitted[0], admitted_traffic_mempool,
			&topo_config);

	// Calculate start and end times
	start_time = rte_get_timer_cycles() + sec_to_hpet(0.2); /* start after last end */
	end_time = start_time + sec_to_hpet(100*1000*1000);

	/*** PATH_SELECTION CORES ***/
	/* set commands */
	path_sel_cmd.q_admitted = q_admitted[0];
	path_sel_cmd.q_path_selected = q_path_selected;

	/* launch admission core */
	if (N_PATH_SEL_CORES > 0)
		rte_eal_remote_launch(exec_path_sel_core, &path_sel_cmd,
				enabled_lcore[FIRST_PATH_SEL_CORE]);

	/*** ADMISSION CORES ***/
	/* initialize core structures */
	for (i = 0; i < N_ADMISSION_CORES; i++) {
		uint16_t lcore_id = enabled_lcore[FIRST_ADMISSION_CORE + i];
		admission_init_core(lcore_id);
	}

	for (i = 0; i < N_ADMISSION_CORES; i++) {
		uint16_t lcore_id = enabled_lcore[FIRST_ADMISSION_CORE + i];

		/* set commands */
		admission_cmd[i].start_time = start_time;
		admission_cmd[i].end_time = end_time;
		admission_cmd[i].admission_core_index = i;
#ifdef EMULATION_ALGO
		admission_cmd[i].start_timeslot = first_time_slot;
#else
		admission_cmd[i].start_timeslot = first_time_slot + i * BATCH_SIZE;
#endif
		/* launch admission core */
		rte_eal_remote_launch(exec_admission_core, &admission_cmd[i], lcore_id);
	}

	/*** LOG CORE ***/
	log_core = new LogCore((uint64_t)(LOG_GAP_SECS * rte_get_timer_hz()),
						   (uint64_t)(Q_LOG_GAP_SECS * rte_get_timer_hz()));
	for (i = 0; i < N_COMM_CORES; i++)
		log_core->add_comm_lcore(enabled_lcore[FIRST_COMM_CORE + i]);

	/* add all admission cores */
	for (i = 0; i < N_ADMISSION_CORES; i++)
		log_core->add_logged_lcore(enabled_lcore[FIRST_ADMISSION_CORE + i]);
#ifdef EMULATION_ALGO
	for (i = 0; i < num_routers(&topo_config); i++)
		log_core->add_queueing_stats(emu_get_queueing_stats(i));
	for (i = 0; i < ALGO_N_CORES; i++)
		log_core->add_drop_stats(emu_get_port_stats(i));
#endif

	/** Benchmark set-up, if we're running a benchmark. Must create benchmark
	 * before we launch the log core. */
#ifdef BENCHMARK_ALGO
	benchmark = new Benchmark(q_admitted, admitted_traffic_mempool,
			N_BENCHMARK_CORES - 1, BENCH_MODE_Q_PER_ENQ_CORE);
//			N_BENCHMARK_CORES - 1, BENCH_MODE_Q_PER_DEQ_CORE);

	for (i = 0; i < N_BENCHMARK_CORES; i++) {
		uint16_t lcore_id = enabled_lcore[FIRST_BENCHMARK_CORE + i];

		/* add to log core */
		log_core->add_logged_lcore(enabled_lcore[FIRST_BENCHMARK_CORE + i]);

		bench_cmd[i].core_index = i;
		bench_cmd[i].benchmark = benchmark;
		benchmark->remote_launch_core(&bench_cmd[i], lcore_id);
	}
#endif

	/* launch log core */
	if (N_LOG_CORES > 0)
		log_core->remote_launch(enabled_lcore[FIRST_LOG_CORE]);

	/*** COMM/STRESS_TEST CORES ***/
	if (IS_STRESS_TEST) {
		launch_stress_test_cores(start_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                         end_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                         first_time_slot, &q_path_selected, &q_admitted[0],
										 admitted_traffic_mempool, &topo_config);
	} else {
		launch_comm_cores(start_time, end_time, first_time_slot, &q_path_selected,
				&q_admitted[0], admitted_traffic_mempool);
	}

	printf("waiting for all cores..\n");
	/** Wait for all cores */
	rte_eal_mp_wait_lcore();

	rte_exit(EXIT_SUCCESS, "Done");
}

/* may be useful later for logging cores */
//	for (i = 0; i < N_CONTROLLER_PORTS; i++) {
//		uint32_t log_core = 1 + i;
//		uint32_t gen_core = 1 + N_CONTROLLER_PORTS + i;
//		uint32_t ctrl_port = i;
//		uint32_t gen_port = N_CONTROLLER_PORTS + i;
//
//		rx_cmds[log_core].start_time = start_time;
//		rx_cmds[log_core].end_time = end_time;
//		/* filename res_grant_TOR_AGG_CORE_PATHS_MEAN-T.csv */
//		snprintf(rx_cmds[log_core].filename, RX_GRANT_MAX_FILENAME_LEN,
//				"results/res_grant_%"PRIu32"_%"PRIu32"_%"PRIu32"_%"PRIu32
//				"_%e_port_%d.csv", tor_n_machines,agg_n_ports,core_n_ports,alloc_n_paths,
//				(double)mean_reqs_per_sec[next_run_index], ctrl_port);
//		rte_eal_remote_launch(exec_rx_grant, &rx_cmds[log_core], log_core);
//
//		rx_cmds[log_core].start_time = start_time;
//		rx_cmds[log_core].end_time = end_time;
//		ether_addr_copy(&port_info[enabled_port[ctrl_port]].eth_addr, &gen_cmds[gen_core].dst_addr);
//		gen_cmds[gen_core].egress_port = enabled_port[gen_port];
//		gen_cmds[gen_core].mean_t_btwn_requests = N_CONTROLLER_PORTS * sec_to_hpet(1.0 / mean_reqs_per_sec[next_run_index]);
//		gen_cmds[gen_core].num_machines = num_machines;
//		rte_eal_remote_launch(exec_traffic_gen, &gen_cmds[gen_core], gen_core);
//
//	}
