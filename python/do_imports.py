
import ctypes
import sys
saved_flags = sys.getdlopenflags()
sys.setdlopenflags(saved_flags | ctypes.RTLD_GLOBAL)
# import dpdk with RTLD_GLOBAL, so it would export its symbols to other PMDs
from dpdk import *
sys.setdlopenflags(saved_flags)
from fastemu import *

params = ["pytest", "-c", "1", "-n", "2", "--no-pci", "--no-huge"]
rte_eal_init(params)

rte_set_log_level(RTE_LOG_DEBUG)

