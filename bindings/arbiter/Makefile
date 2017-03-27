# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk
EMU_DIR = ${PWD}/../../src/emulation
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
			  $(ARBITER_DIR)/stress_test_core.h \
			  $(GRAPH_ALGO_DIR)/algo_config.h \
			  $(GRAPH_ALGO_DIR)/admissible.h

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
CFLAGS += -I${ARBITER_DIR} 
CFLAGS += -I${EMU_DIR}
#CFLAGS += -g -O1
CFLAGS += -g
CFLAGS += -march=core2
CFLAGS += $(CMD_LINE_CFLAGS)

CDEFINES = 
CDEFINES += -DFASTPASS_CONTROLLER
#CDEFINES += -DCONFIG_IP_FASTPASS_DEBUG
CDEFINES += -DLOG_TO_STDOUT
CDEFINES += -DPRINT_CONN_LOG_TO_STDOUT
CDEFINES += -DNO_HW_CHECKSUM
#CDEFINES += -DINTEL_NICS
#CDEFINES += -DPIM_SINGLE_ADMISSION_CORE
#CDEFINES += -DNO_ATOMIC
CDEFINES += -DNDEBUG
#CDEFINES += -DPARALLEL_ALGO
#CDEFINES += -DPIPELINED_ALGO
CDEFINES += -DEMULATION_ALGO
#CCDEFINES += -DEMU_NO_BATCH_CALLS

# use drop tail if nothing else is specified
ifeq ($(EMU_RTR_FLAGS),)
	CDEFINES += -DDROP_TAIL
else
	CDEFINES += $(EMU_RTR_FLAGS)
endif

CFLAGS += $(CDEFINES)

# C++ flags
CPPFLAGS += $(CFLAGS)
CPPFLAGS += -D__STDC_LIMIT_MACROS

%_wrap.cc: %.i $(WRAP_H_DEPS)
	swig -c++ -python $(CDEFINES) -I$(RTE_SDK)/$(RTE_TARGET)/include -I$(ARBITER_DIR) -o $@ $< 
CPPFLAGS_fastpass_wrap_cc.o = -I /usr/include/python2.7/ 


# linker settings to work with C++
LD = g++
LDFLAGS += -lstdc++
LINK_USING_CC = 0

# don't use KNI
CONFIG_RTE_LIBRTE_KNI=n



include $(RTE_SDK)/mk/rte.extshared.mk
