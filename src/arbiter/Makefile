# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk
EMU_DIR = ../../../fastpass-private/src/emulation

# binary name
APP = fast

SRCS-y := main.c \
          control.c \
          comm_core.c \
          seq_admission_core.c \
          path_sel_core.c \
          log_core.c \
          stress_test_core.c \
          ../protocol/fpproto.c \
          ../graph-algo/admissible_traffic.c \
          ../graph-algo/path_selection.c \
          ../graph-algo/euler_split.c \
          emu_admission_core.c \
          $(EMU_DIR)/admissible_log.c \
          $(EMU_DIR)/drop_tail.c \
          $(EMU_DIR)/emulation.c \
          $(EMU_DIR)/endpoint.c \
          $(EMU_DIR)/packet.c \
          $(EMU_DIR)/router.c \

#          pim_admission_core.c \
#          ../grant-accept/pim.c

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

include $(RTE_SDK)/mk/rte.extapp.mk
