#!/bin/bash

# build several emulators with different numbers of racks, all with 1 Core switch
# first arg is "mlx" if we should compile for mellanox, empty otherwise

n_racks=( 2 3 4 5 6 7 8 )

for racks in "${n_racks[@]}"
do
    cores=$((racks + 1))
    echo "Building emulator with $racks racks and $cores cores"

    if [ "$#" -ne 1 ]; then
        # build for intel
        make clean && make CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks -j22
    elif [ "$1" = "mlx" ]; then
        # build for mellanox
        make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks -j22
    else
        echo "unrecognized argument $1"
    fi

    cp build/fast build/fast_"$racks"_racks
done

# copy build arbiters to build directory for deployment to arbiter machines
cp build/fast $BUILD
cp build/fast_* $BUILD
cp run_mlx.sh $BUILD
