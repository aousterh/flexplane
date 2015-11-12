# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk
EMU_DIR = ../../../fastpass-private/src/emulation

# binary name, if not specified elsewhere
ifeq ($(APP),)
	APP = fast
endif

SRCS-y := main.c \
          control.cc \
          comm_core.c \
          seq_admission_core.c \
          path_sel_core.c \
          log_core.cc \
          stress_test_core.c \
          ../protocol/fpproto.c \
          ../graph-algo/admissible_traffic.c \
          ../graph-algo/path_selection.c \
          ../graph-algo/euler_split.c \
          benchmark.cc \
          benchmark_core.cc \
          emu_admission_core.cc \
          $(EMU_DIR)/queue_managers/drop_tail.cc \
          $(EMU_DIR)/queue_managers/drop_tail_tso.cc \
          $(EMU_DIR)/queue_managers/red.cc \
          $(EMU_DIR)/queue_managers/dctcp.cc \
          $(EMU_DIR)/queue_managers/pfabric_qm.cc \
          $(EMU_DIR)/queue_managers/lstf_qm.cc \
          $(EMU_DIR)/schedulers/hull_sched.cc \
          $(EMU_DIR)/simple_endpoint.cc \
          $(EMU_DIR)/emulation.cc \
          $(EMU_DIR)/emulation_core.cc \
          $(EMU_DIR)/emulation_c_compat.cc \
          $(EMU_DIR)/endpoint_group.cc \
          $(EMU_DIR)/router.cc \
          $(EMU_DIR)/drivers/EndpointDriver.cc \
          $(EMU_DIR)/drivers/RouterDriver.cc \

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
CFLAGS += -I${PWD}
CFLAGS += -I$(PWD)/$(EMU_DIR)
#CFLAGS += -g -O1
CFLAGS += -g
CFLAGS += -DNDEBUG
CFLAGS += -march=core2
#CFLAGS += -DPARALLEL_ALGO
#CFLAGS += -DPIPELINED_ALGO
CFLAGS += -DEMULATION_ALGO
#CFLAGS += -DUSE_TSO
#CFLAGS += -DBENCHMARK_ALGO
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

include $(RTE_SDK)/mk/rte.extapp.mk
