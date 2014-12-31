#!/usr/bin/python


from dpdk import *

print "eal init:", rte_eal_init(["pytest", "-c", "6", "-n", "2", 
                                      "--no-huge"]) #, "--no-pci"])

enabled_cores = [i for i in xrange(RTE_MAX_LCORE) if rte_lcore_is_enabled(i)]
print "enabled_lcores:", enabled_cores

print "lcore_id", rte_lcore_id()

print "core 2 is on socket %d" % rte_lcore_to_socket_id(2)


pool = PacketPool("pktpool", 1024, 2048, 128, 0, 0, 0)

print "pool count:", pool.count()
print "allocated packet:", pool.alloc()
print "pool count:", pool.count()

print "rte_log debug"
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

