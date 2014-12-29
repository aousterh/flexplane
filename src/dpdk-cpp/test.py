#!/usr/bin/python


from dpdk import *

print "eal init:", rte_eal_init(["pytest", "-c", "6", "-n", "2", 
                                      "--no-huge", "--no-pci"])

enabled_cores = [i for i in xrange(RTE_MAX_LCORE) if rte_lcore_is_enabled(i)]
print "enabled_lcores:", enabled_cores

print "lcore_id", rte_lcore_id()

print "core 2 is on socket %d" % rte_lcore_to_socket_id(2)


pool = PacketPool("pktpool", 1024, 2048, 128, 0, 0, 0)
print pool

print pool.count()
print pool.alloc()
print dir(pool.alloc())
print pool.count()
