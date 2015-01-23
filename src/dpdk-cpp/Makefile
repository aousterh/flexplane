# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
SHARED = _dpdk.so

WRAP_H_DEPS = EthernetDevice.h \
			  MemPool.h \
			  PacketPool.h \
			  Power.h \
			  Ring.h

%_wrap.cc: %.i $(WRAP_H_DEPS)
	swig -c++ -python -I$(RTE_SDK)/$(RTE_TARGET)/include -o $@ $< 
CPPFLAGS_dpdk_wrap_cc.o = -I /usr/include/python2.7/


SRCS-y := EthernetDevice.cc 	\
	PacketPool.cc				\
	MemPool.cc				\
	dpdk_wrap.cc

#CFLAGS += -std=c++11
#CFLAGS += -Wno-literal-suffix
CFLAGS += -O3 
#CFLAGS += $(WERROR_FLAGS)
CFLAGS += -DFASTPASS_CONTROLLER 
#CFLAGS += -DCONFIG_IP_FASTPASS_DEBUG
CFLAGS += -DLOG_TO_STDOUT
CFLAGS += -DPRINT_CONN_LOG_TO_STDOUT 
#CFLAGS += -DPIM_SINGLE_ADMISSION_CORE
#CFLAGS += -DNO_ATOMIC
CFLAGS += -I${PWD} 
CFLAGS += -I$(PWD)/$(EMU_DIR)
CFLAGS += -I/usr/include/python2.7
#CFLAGS += -g -O1
CFLAGS += -g 
CFLAGS += -DNDEBUG
CFLAGS += -march=core2
#CFLAGS += -DPARALLEL_ALGO
#CFLAGS += -DPIPELINED_ALGO
CFLAGS += -DEMULATION_ALGO
CFLAGS += $(CMD_LINE_CFLAGS)
#CFLAGS += -fvisibility=hidden

CPPFLAGS = $(CFLAGS)

# linker settings to work with C++
LD = g++
LDFLAGS += -lstdc++
CC = g++

# whole-archive needed for correct linking of __attribute__((constructor)), used
#  by DPDK to register PMDs.
LDLIBS = --whole-archive -lintel_dpdk --no-whole-archive -lpcap
#LDLIBS = --whole-archive -lrte_mbuf -lethdev -lrte_malloc -lrte_mempool -lrte_ring -lrte_eal -lrte_pmd_ixgbe --no-whole-archive
include $(RTE_SDK)/mk/rte.extshared.mk
