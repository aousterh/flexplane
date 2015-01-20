# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk
EMU_DIR = ${PWD}/../../../fastpass-private/src/emulation
ARBITER_DIR = ${PWD}/../../src/arbiter
PROTOCOL_DIR = ${PWD}/../../src/protocol
GRAPH_ALGO_DIR = ${PWD}/../../src/graph-algo

# binary name
SHARED = _fastpass.so

WRAP_H_DEPS = $(ARBITER_DIR)/control.h \
			  $(ARBITER_DIR)/comm_core.h \
			  $(ARBITER_DIR)/admission_core.h \
			  $(ARBITER_DIR)/admission_core_common.h \
			  $(ARBITER_DIR)/path_sel_core.h \
			  $(ARBITER_DIR)/log_core.h \
			  $(ARBITER_DIR)/stress_test_core.h			  

%_wrap.cc: %.i $(WRAP_H_DEPS)
	swig -c++ -python -I$(RTE_SDK)/$(RTE_TARGET)/include -I$(ARBITER_DIR) -o $@ $< 
CPPFLAGS_fastpass_wrap_cc.o = -I /usr/include/python2.7/ 

SRCS-y := $(ARBITER_DIR)/main.c \
          $(ARBITER_DIR)/control.c \
          $(ARBITER_DIR)/comm_core.c \
          $(ARBITER_DIR)/seq_admission_core.c \
          $(ARBITER_DIR)/path_sel_core.c \
          $(ARBITER_DIR)/log_core.c \
          $(ARBITER_DIR)/stress_test_core.c \
          $(PROTOCOL_DIR)/fpproto.c \
          $(GRAPH_ALGO_DIR)/admissible_traffic.c \
          $(GRAPH_ALGO_DIR)/path_selection.c \
          $(GRAPH_ALGO_DIR)/euler_split.c \
          $(ARBITER_DIR)/emu_admission_core.cc \
          $(EMU_DIR)/queue_managers/drop_tail.cc \
          $(EMU_DIR)/queue_managers/red.cc \
          $(EMU_DIR)/queue_managers/dctcp.cc \
          $(EMU_DIR)/simple_endpoint.cc \
          $(EMU_DIR)/emulation.cc \
          $(EMU_DIR)/endpoint_group.cc \
          $(EMU_DIR)/router.cc \
          $(EMU_DIR)/drivers/EndpointDriver.cc \
          $(EMU_DIR)/drivers/RouterDriver.cc \
          fastpass_wrap.cc
          
#          pim_admission_core.c \
#          ../grant-accept/pim.c

CFLAGS += -O3
#CFLAGS += $(WERROR_FLAGS)
CFLAGS += -DFASTPASS_CONTROLLER
#CFLAGS += -DCONFIG_IP_FASTPASS_DEBUG
CFLAGS += -DLOG_TO_STDOUT
CFLAGS += -DPRINT_CONN_LOG_TO_STDOUT
CFLAGS += -DNO_HW_CHECKSUM
#CFLAGS += -DINTEL_NICS
#CFLAGS += -DPIM_SINGLE_ADMISSION_CORE
#CFLAGS += -DNO_ATOMIC
CFLAGS += -I${ARBITER_DIR} 
CFLAGS += -I${EMU_DIR}
#CFLAGS += -g -O1
CFLAGS += -g
CFLAGS += -DNDEBUG
CFLAGS += -march=core2
#CFLAGS += -DPARALLEL_ALGO
#CFLAGS += -DPIPELINED_ALGO
CFLAGS += -DEMULATION_ALGO
#CCFLAGS += -DEMU_NO_BATCH_CALLS
CFLAGS += $(CMD_LINE_CFLAGS)

# use drop tail if nothing else is specified
ifeq ($(EMU_RTR_FLAGS),)
	CFLAGS += -DDROP_TAIL
else
	CFLAGS += $(EMU_RTR_FLAGS)
endif

# C++ flags
CPPFLAGS += $(CFLAGS)
CPPFLAGS += -D__STDC_LIMIT_MACROS

# linker settings to work with C++
LD = g++
LDFLAGS += -lstdc++

# don't use KNI
CONFIG_RTE_LIBRTE_KNI=n

include $(RTE_SDK)/mk/rte.extshared.mk
