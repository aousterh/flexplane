#!/usr/bin/python


import ctypes
import sys
saved_flags = sys.getdlopenflags()
sys.setdlopenflags(saved_flags | ctypes.RTLD_GLOBAL)
# import dpdk with RTLD_GLOBAL, so it would export its symbols to other PMDs
from dpdk import *
sys.setdlopenflags(saved_flags)

from fastpass import *
import fastpass
import dpdk

print dir(fastpass)
print dir(dpdk)

import time
from copy import copy


params = ["pytest", "-c", "e", "-n", "2", "-m", "512"]
params += ["--vdev", "eth_pcap0,iface=eth0"]
print "eal init:", rte_eal_init(params) #, "--no-pci"])

enabled_cores = [i for i in xrange(RTE_MAX_LCORE) if rte_lcore_is_enabled(i)]
print "enabled_lcores:", enabled_cores

pool = PacketPool("pktpool", 1024, 2048, 128, 0, 0)

rte_set_log_level(RTE_LOG_DEBUG)
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
    first_time_slot = ((now + INIT_MAX_TIME_NS) * TIMESLOT_MUL) >> TIMESLOT_SHIFT;
    print ("CONTROL: now %lu first time slot will be %lu" % (now, first_time_slot))

    #/*** LOGGING OUTPUT ***/
    rte_openlog_stdout()

    #/*** GLOBAL INIT ***/
    #/* initialize comm core global data */
    comm_init_global_structs(first_time_slot);

    #/* create q_admitted_out */
    q_admitted = Ring("q_admitted", 2 * 128, 0, 0);

    #/* create q_path_selected_out */
    q_path_selected = Ring("q_path_selected", 2 * 128, 0, 0);

    #/* initialize admission core global data */
    admission_init_global(q_admitted.get());

    #// Calculate start and end times
    start_time = rte_get_timer_cycles() + sec_to_hpet(0.2); #/* start after last end */
    end_time = start_time + sec_to_hpet(100*1000*1000);

    remaining_cores = copy(enabled_cores)
    remaining_cores = [x for x in enabled_cores if x != rte_lcore_id()]

#     #/*** PATH_SELECTION CORES ***/
#     #/* set commands */
#     path_sel_cmd.q_admitted = q_admitted;
#     path_sel_cmd.q_path_selected = q_path_selected;
# 
#     #/* launch admission core */
#     if (N_PATH_SEL_CORES > 0)
#         rte_eal_remote_launch(exec_path_sel_core, &path_sel_cmd,
#                 enabled_lcore[FIRST_PATH_SEL_CORE]);

    #/*** ADMISSION CORES ***/
    #/* initialize core structures */
    admission_cores = remaining_cores[:N_ADMISSION_CORES]
    remaining_cores = remaining_cores[N_ADMISSION_CORES:]
    
    for lcore_id in admission_cores:
        admission_init_core(lcore_id)

    admission_cmds = []
    for i, lcore_id in enumerate(admission_cores):
        cmd = admission_core_cmd()
        
        #/* set commands */
        cmd.start_time = start_time;
        cmd.end_time = end_time;
        cmd.admission_core_index = i;
        cmd.start_timeslot = first_time_slot + i * BATCH_SIZE;

        #/* launch admission core */
        rte_eal_remote_launch(exec_admission_core_funcptr, cmd, lcore_id);
        
        # save commands. if garbage collector gets hold of them we're in trouble
        admission_cmds.append(cmd)

    #/*** LOG CORE ***/
    log_cmd = log_core_cmd()
    log_cmd.start_time = start_time;
    log_cmd.end_time = end_time;
    log_cmd.log_gap_ticks = int(LOG_GAP_SECS * rte_get_timer_hz());
    log_cmd.q_log_gap_ticks = int(Q_LOG_GAP_SECS * rte_get_timer_hz());

    #/* launch log core */
    if (N_LOG_CORES > 0):
        rte_eal_remote_launch(exec_log_core, log_cmd, remaining_cores.pop(0))

    #/*** COMM/STRESS_TEST CORES ***/
    if (IS_STRESS_TEST):
        launch_stress_test_cores(start_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                 end_time + STRESS_TEST_START_GAP_SEC * rte_get_timer_hz(),
                                 first_time_slot, q_path_selected.get(), q_admitted.get());
    else:
        comm_cmd = comm_core_cmd()
    
        # Set commands
        comm_cmd.start_time = start_time;
        comm_cmd.end_time = end_time;
        if (N_PATH_SEL_CORES > 0):
            comm_cmd.q_allocated = q_path_selected.get()
        else: 
            comm_cmd.q_allocated = q_admitted.get();
    
        #/* initialize comm core on this core */
        comm_init_core(rte_lcore_id(), first_time_slot);
    
        #/** Run the controller on this core */
        exec_comm_core(comm_cmd);

    print "waiting for all cores.."
    #/** Wait for all cores */
    rte_eal_mp_wait_lcore();

    rte_exit(EXIT_SUCCESS, "Done");


launch_cores()