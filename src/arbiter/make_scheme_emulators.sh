#!/bin/bash

# build several emulators with different schemes, with different numbers of
# racks

n_racks=( 1 )
schemes=( DROP_TAIL RED DCTCP HULL_SCHED ROUND_ROBIN PFABRIC \
                    PRIO_BY_FLOW_QUEUEING LSTF)

for scheme in "${schemes[@]}"
do
    for racks in "${n_racks[@]}"
    do
        if [ $racks = "1" ]; then
            cores=1
            comm_cores=1
        else
            cores=$((racks + 1))
            comm_cores=2
        fi

        echo "Building emulator with scheme $scheme and $racks racks ($cores cores)"
        prefix="-D"
        fast_name=build/fast_"$scheme"_"$racks"_racks
        rm $fast_name

        if [ $scheme = "PRIO_BY_FLOW_QUEUEING" ]; then
           flow_shift=2
        else
            flow_shift=0
        fi

        make clean && make CMD_LINE_CFLAGS+=-DALGO_N_CORES=$cores \
                           CMD_LINE_CFLAGS+=-DEMU_NUM_RACKS=$racks \
                           CMD_LINE_CFLAGS+=-DN_COMM_CORES=$comm_cores \
                           CMD_LINE_CFLAGS+=-DFLOW_SHIFT=$flow_shift \
                           EMU_RTR_FLAGS=$prefix$scheme -j22

        cp build/fast $fast_name
    done
done
