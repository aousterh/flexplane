# Based on the DPDK Programmer Guide November 2012 (Ch 15.3.1)

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = fast

SRCS-y := main.c control.c comm_core.c ../kernel-mod/fpproto.c

CFLAGS += -O3 
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -DFASTPASS_CONTROLLER
#CFLAGS += -g -O1
CFLAGS += -DNDEBUG

include $(RTE_SDK)/mk/rte.extapp.mk