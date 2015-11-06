#!/bin/bash

# run several emulators with different numbers of racks, all with 1 Core switch
# first arg is "mlx" if we should compile for mellanox, empty otherwise

n_racks=( 2 3 4 5 6 7 8 )
n_comms=2

# create a directory for logging
mkdir /tmp/aousterh-logs

# shield cpus appropriately
sudo cset shield -c 1-14

for racks in "${n_racks[@]}"
do
    OUT_FILE="emu_output_${racks}_racks_${n_comms}_comms.out"
    ERR_FILE="emu_output_${racks}_racks_${n_comms}_comms.err"
    echo "running topo with $racks racks, outfile: $OUT_FILE"

    FAST=build/fast_"$racks"_racks_"$n_comms"_comms

    if [ "$#" -ne 1 ]; then
        sudo cset shield -e -- $FAST -c 7ffe -n 3 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE
    elif [ "$1" = "mlx" ]; then
        # run on mellanox
        sudo cset shield -e -- $FAST -c 7ffe -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > $OUT_FILE 2> $ERR_FILE
    else
        echo "unrecognized argument $1"
    fi

    grep "30 seconds" $OUT_FILE

    # pause to allow huge pages to become available again
    sleep 1s
done

sudo cset shield --reset
