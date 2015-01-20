#!/usr/bin/python


import ctypes
import sys
saved_flags = sys.getdlopenflags()
sys.setdlopenflags(saved_flags | ctypes.RTLD_GLOBAL)
# import dpdk with RTLD_GLOBAL, so it would export its symbols to other PMDs
from dpdk import *
sys.setdlopenflags(saved_flags)

from fastpass import *

import time


params = ["pytest", "-c", "6", "-n", "2", "--no-huge"]
params += ["--vdev", "eth_pcap0,iface=eth0"]
print "eal init:", rte_eal_init(params) #, "--no-pci"])

enabled_cores = [i for i in xrange(RTE_MAX_LCORE) if rte_lcore_is_enabled(i)]
print "enabled_lcores:", enabled_cores

pool = PacketPool("pktpool", 1024, 2048, 128, 0, 0, 0)

rte_set_log_level(RTE_LOG_DEBUG)
rte_openlog_stdout()
print "cur log level", rte_log_cur_msg_loglevel()

print "pci probe", rte_eal_pci_probe()
print "pci driver list"
dump_pci_drivers()
# print "pci dump"
# rte_eal_pci_dump()

print "rte_eth_dev_count", rte_eth_dev_count()

eth_config = eth_conf()
print "rx.hw_ip_checksum =",eth_config.rx().hw_ip_checksum()
eth_dev = EthernetDevice(0, 1, 1, eth_config)

txconf = eth_txconf()
eth_dev.tx_queue_setup(0, 512, 0, txconf)

rxconf = eth_rxconf()
eth_dev.rx_queue_setup(0, 128, 0, rxconf, pool.get())

eth_dev.start()
eth_dev.promiscuous_enable()
link = eth_dev.link_get_nowait()
if (link.link_status):
    duplex_opts = ["full-duplex", "half-duplex"]
    duplex_str = duplex_opts[link.link_duplex == ETH_LINK_FULL_DUPLEX]
    print "Port 0 Link Up - speed %u Mbps - %s" \
           % (link.link_speed, duplex_str)
else:
    print "Port 0 Link Down"




def launch_cores():
    #/* variables */
    #//struct comm_core_cmd comm_cores[RTE_MAX_LCORE];
#     uint64_t start_time;
#     static uint64_t end_time;
#     int i; (void)i;
#     struct admission_core_cmd admission_cmd[N_ADMISSION_CORES];
#     struct path_sel_core_cmd path_sel_cmd;
#     struct log_core_cmd log_cmd;
#     uint64_t first_time_slot;
#     uint64_t now;
#     struct rte_ring *q_admitted;
#     struct rte_ring *q_path_selected;

    benchmark_cost_of_get_time();
    #/* decide what the first time slot to be output is */
    now = fp_get_time_ns();
    print now, dir(now)
    print INIT_MAX_TIME_NS
    first_time_slot = ((now + INIT_MAX_TIME_NS) * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
    CONTROL_INFO("now %lu first time slot will be %lu\n", now, first_time_slot);

"""

    #/*** LOGGING OUTPUT ***/
#ifdef LOG_TO_STDOUT
    rte_openlog_stream(stdout);
#endif

    #/*** GLOBAL INIT ***/
    #/* initialize comm core global data */
    comm_init_global_structs(first_time_slot);

    #/* create q_admitted_out */
    q_admitted = rte_ring_create("q_admitted",
            2 * ADMITTED_TRAFFIC_MEMPOOL_SIZE, 0, 0);
    if (q_admitted == NULL)
        rte_exit(EXIT_FAILURE,
                "Cannot init q_admitted: %s\n", rte_strerror(rte_errno));

    #/* create q_path_selected_out */
    q_path_selected = rte_ring_create("q_path_selected",
            2 * ADMITTED_TRAFFIC_MEMPOOL_SIZE, 0, 0);
    if (q_path_selected == NULL)
        rte_exit(EXIT_FAILURE,
                "Cannot init q_path_selected: %s\n", rte_strerror(rte_errno));

    #/* initialize admission core global data */
    admission_init_global(q_admitted);

    #// Calculate start and end times
    start_time = rte_get_timer_cycles() + sec_to_hpet(0.2); /* start after last end */
    end_time = start_time + sec_to_hpet(100*1000*1000);

    #/*** PATH_SELECTION CORES ***/
    #/* set commands */
    path_sel_cmd.q_admitted = q_admitted;
    path_sel_cmd.q_path_selected = q_path_selected;

    #/* launch admission core */
    if (N_PATH_SEL_CORES > 0)
        rte_eal_remote_launch(exec_path_sel_core, &path_sel_cmd,
                enabled_lcore[FIRST_PATH_SEL_CORE]);

    #/*** ADMISSION CORES ***/
    #/* initialize core structures */
    for (i = 0; i < N_ADMISSION_CORES; i++) {
        uint16_t lcore_id = enabled_lcore[FIRST_ADMISSION_CORE + i];
        admission_init_core(lcore_id);
    }

    for (i = 0; i < N_ADMISSION_CORES; i++) {
        uint16_t lcore_id = enabled_lcore[FIRST_ADMISSION_CORE + i];

        #/* set commands */
        admission_cmd[i].start_time = start_time;
        admission_cmd[i].end_time = end_time;
        admission_cmd[i].admission_core_index = i;
        admission_cmd[i].start_timeslot = first_time_slot + i * BATCH_SIZE;

        #/* launch admission core */
        rte_eal_remote_launch(exec_admission_core, &admission_cmd[i], lcore_id);
    }

    #/*** LOG CORE ***/
    log_cmd.log_gap_ticks = (uint64_t)(LOG_GAP_SECS * rte_get_timer_hz());
    log_cmd.q_log_gap_ticks = (uint64_t)(Q_LOG_GAP_SECS * rte_get_timer_hz());
    log_cmd.start_time = start_time;
    log_cmd.end_time = end_time;

    #/* launch log core */
    if (N_LOG_CORES > 0)
        rte_eal_remote_launch(exec_log_core, &log_cmd,
                enabled_lcore[FIRST_LOG_CORE]);

    #/*** COMM/STRESS_TEST CORES ***/
    if (IS_STRESS_TEST) {
        launch_stress_test_cores(start_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                         end_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                         first_time_slot, q_path_selected, q_admitted);
    } else {
        launch_comm_cores(start_time, end_time, first_time_slot, q_path_selected,
                q_admitted);
    }

    printf("waiting for all cores..\n");
    #/** Wait for all cores */
    rte_eal_mp_wait_lcore();

    rte_exit(EXIT_SUCCESS, "Done");
}

"""

launch_cores()