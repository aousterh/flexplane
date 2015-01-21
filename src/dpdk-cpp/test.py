#!/usr/bin/python


import ctypes
import sys
saved_flags = sys.getdlopenflags()
sys.setdlopenflags(saved_flags | ctypes.RTLD_GLOBAL)
# import dpdk with RTLD_GLOBAL, so it would export its symbols to other PMDs
from dpdk import *
sys.setdlopenflags(saved_flags)

import time
import binascii
import dpkt

params = ["pytest", "-c", "6", "-n", "2", "--no-huge"]
params += ["--vdev", "eth_pcap0,iface=eth0"]
print "eal init:", rte_eal_init(params) #, "--no-pci"])

enabled_cores = [i for i in xrange(RTE_MAX_LCORE) if rte_lcore_is_enabled(i)]
print "enabled_lcores:", enabled_cores

print "lcore_id", rte_lcore_id()

print "core 2 is on socket %d" % rte_lcore_to_socket_id(2)


pool = PacketPool("pktpool", 1024, 2048, 128, 0, 0)

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


for i in xrange(10):
    RX_BURST_SIZE = 64
    pkts = new_mbuf_array(RX_BURST_SIZE)
    nb_rx = rte_eth_rx_burst(0,0,pkts,RX_BURST_SIZE)
    print "rx", nb_rx 
    for i in xrange(nb_rx):
        print "pkt %d" % i
        data = mbuf_array_getitem(pkts, i).pkt_data()
        pkt = dpkt.ethernet.Ethernet(data)
        print repr(pkt)
        
        print binascii.hexlify(data)
    
    print "sleep(1)"
    time.sleep(1)
    