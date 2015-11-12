#!/bin/bash

# build several emulators with different numbers of racks, all with 1 Core switch
# first arg is "mlx" if we should compile for mellanox, empty otherwise

n_racks=( 1 2 3 4 5 6 7 8 )

for racks in "${n_racks[@]}"
do
    # set number of cores and number of comms
#    cores=$((racks + 1))
    cores=$((racks))
    if [ $racks = 1 ]; then
        comms=1
    else
        comms=2
    fi

    echo "Building emulator with $racks racks, $cores cores, and $comms comms"
    
    
    if [ "$#" -ne 1 ]; then
        # build for intel
        make clean && make CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks CMD_LINE_CFLAGS+=-DN_COMM_CORES=$comms APP=fast_"$racks"_racks_"$comms"_comms -j22
    elif [ "$1" = "mlx" ]; then
        # build for mellanox
        make clean && make CONFIG_RTE_LIBRTE_PMD_PCAP=y CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks CMD_LINE_CFLAGS+=-DN_COMM_CORES=$comms APP=fast_"$racks"_racks_"$comms"_comms -j22
    else
        echo "unrecognized argument $1"
    fi

done

# copy build arbiters to build directory for deployment to arbiter machines
cp build/fast $BUILD
cp build/fast_* $BUILD
cp run_mlx.sh $BUILD
