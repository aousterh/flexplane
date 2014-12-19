# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
SHARED = _dpdk.so

%_wrap.cc: %.i EthernetDevice.h
	swig -c++ -python -I$(RTE_SDK)/$(RTE_TARGET)/include -o $@ $< 
CFLAGS_dpdk_wrap_cc.o = -I /usr/include/python2.7/


SRCS-y := EthernetDevice.cc \
	dpdk_wrap.cc

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
#CFLAGS += -g -O1
CFLAGS += -g 
CFLAGS += -DNDEBUG
CFLAGS += -march=core2
#CFLAGS += -DPARALLEL_ALGO
#CFLAGS += -DPIPELINED_ALGO
CFLAGS += -DEMULATION_ALGO
CFLAGS += $(CMD_LINE_CFLAGS)

# linker settings to work with C++
LD = g++
LDFLAGS += -lstdc++
CC = g++

include $(RTE_SDK)/mk/rte.extshared.mk
