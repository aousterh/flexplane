#!/bin/bash

# compile one arbiter of each type for running experiments

# make arbiter that runs RED
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DRED -j22
mv build/fast build/fast_red

# make arbiter that runs DCTCP
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DDCTCP -j22
mv build/fast build/fast_dctcp

# make arbiter that runs priority queueing
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DPRIO_QUEUEING -j22
mv build/fast build/fast_prio

# make arbiter that runs priority queueing by flow
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DPRIO_BY_FLOW_QUEUEING -j22
mv build/fast build/fast_prio_by_flow

# make arbiter that runs round robin
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DROUND_ROBIN -j22
mv build/fast build/fast_rr

# make arbiter that runs HULL as a scheduler
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DHULL_SCHED -j22
mv build/fast build/fast_hull_sched

# make arbiter that runs drop tail. drop tail is the default and must come last.
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DDROP_TAIL -j22
cp build/fast build/fast_drop_tail

# copy build arbiters to build directory for deployment to arbiter machines
cp build/fast $BUILD
cp build/fast_* $BUILD
cp run_mlx.sh $BUILD
