# Macros
CC = g++
#CC = gcc
#CC = icc
EMU_DIR = ../../../fastpass-private/src/emulation
CCFLAGS = -g
CCFLAGS += -DNDEBUG
CCFLAGS += -O3
CCFLAGS += -march=core2
#CCFLAGS += -O0
CCFLAGS += -DNO_DPDK -DALGO_N_CORES=1 
CCFLAGS += -DFASTPASS_CONTROLLER
#CCFLAGS += -DPARALLEL_ALGO
#CCFLAGS += -DPIPELINED_ALGO
CCFLAGS += -DEMULATION_ALGO
#CCFLAGS += -DEMU_NO_BATCH_CALLS
#CCFLAGS += -debug inline-debug-info
#CCFLAGS += -DASM_GOTO_UNSUPPORTED
CCFLAGS += -DAGGREGATE_STATISTICS
CCFLAGS += -I$(PWD)/$(EMU_DIR) -I.
CCFLAGS += -I../arbiter
LDFLAGS = -lm
#LDFLAGS = -debug inline-debug-info

# Pattern rule
%.o: %.c
	$(CC) $(CCFLAGS) -c $<
%.emu.o: $(EMU_DIR)/%.cc
	$(CC) $(CCFLAGS) -c $< -o $@
%.emu_qm.o: $(EMU_DIR)/queue_managers/%.cc
	$(CC) $(CCFLAGS) -c $< -o $@
%.emu_drv.o: $(EMU_DIR)/drivers/%.cc
	$(CC) $(CCFLAGS) -c $< -o $@


# Dependency rules for non-file targets
all: test_euler_split benchmark_graph_algo test_bin_computation rdtsc microbench
clean:
	rm -f test_euler_split benchmark_graph_algo test_bin_computation rdtsc microbench *.o *~
	cd $(EMU_DIR); make clean

# Dependency rules for file target
test_euler_split: test_euler_split.o euler_split.o
	$(CC) $< euler_split.o -o $@ $(LDFLAGS)

#benchmark_graph_algo: benchmark_graph_algo.o admissible_traffic.o path_selection.o euler_split.o ../grant-accept/pim_admissible_traffic.o ../grant-accept/pim.o
#	$(CC) $< admissible_traffic.o path_selection.o euler_split.o ../grant-accept/pim_admissible_traffic.o ../grant-accept/pim.o -o $@ $(LDFLAGS)

benchmark_graph_algo: benchmark_graph_algo.o admissible_traffic.o path_selection.o euler_split.o emulation.emu.o endpoint_group.emu.o drop_tail.emu_qm.o red.emu_qm.o dctcp.emu_qm.o hull.emu_qm.o simple_endpoint.emu.o router.emu.o EndpointDriver.emu_drv.o RouterDriver.emu_drv.o
	$(CC) $^ -o $@ $(LDFLAGS)

#benchmark_sjf: benchmark_sjf.o admissible_traffic_sjf.o path_selection.o euler_split.o
#	$(CC) $< admissible_traffic_sjf.o path_selection.o euler_split.o -o $@ $(LDFLAGS)

test_bin_computation: test_bin_computation.o
	$(CC) $< -o $@ $(LDFLAGS)

microbench: microbench.o
	$(CC) $< -o $@ $(LDFLAGS)

rdtsc: rdtsc.o
