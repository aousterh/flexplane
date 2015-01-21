#!/bin/bash

# compile one arbiter of each type for running experiments

# make arbiter that runs RED
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DRED -j22
mv build/fast build/fast_red

# make arbiter that runs DCTCP
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DDCTCP -j22
mv build/fast build/fast_dctcp

# make arbiter that runs drop tail
make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y EMU_RTR_FLAGS=-DDROP_TAIL -j22
cp build/fast build/fast_drop_tail
