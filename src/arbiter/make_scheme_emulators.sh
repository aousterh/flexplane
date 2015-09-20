#!/bin/bash

# build several emulators with different schemes, all with 1 racks of machines
# first arg is "mlx" if we should compile for mellanox, empty otherwise

racks=1
schemes=( DROP_TAIL RED DCTCP HULL_SCHED ROUND_ROBIN PFABRIC )

for scheme in "${schemes[@]}"
do
    cores=1
    echo "Building emulator with scheme $scheme and $racks racks"
    prefix="-D"
    fast_name=build/fast_"$scheme"_1_rack
    rm $fast_name
    
    if [ "$#" -ne 1 ]; then
        # build for intel
        make clean && make CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores EMU_RTR_FLAGS=$prefix$scheme -j22
    elif [ "$1" = "mlx" ]; then
        # build for mellanox
        make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks -j22
    else
        echo "unrecognized argument $1"
    fi

    cp build/fast $fast_name
done
