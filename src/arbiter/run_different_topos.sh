#!/bin/bash

# run several emulators with different numbers of racks, all with 1 Core switch
# first arg is "mlx" if we should compile for mellanox, empty otherwise

n_racks=( 1 2 3 4 5 6 7 8 )
n_iterations=5
description="multiple_admitted_mempools"

# create a directory for logging
mkdir /tmp/aousterh-logs

# shield cpus appropriately
sudo cset shield -c 1-14

OUT_SUMMARY="emu_output_summary_${description}.csv"
echo "racks,comms,index,tput,label" > $OUT_SUMMARY

for i in `seq 1 $n_iterations`;
do
    for racks in "${n_racks[@]}"
    do
        if [ $racks = 1 ]; then
            comms=1
        else
            comms=2
        fi
        
        OUT_FILE="emu_output_${racks}_racks_${comms}_comms_${i}.out"
        ERR_FILE="emu_output_${racks}_racks_${comms}_comms_${i}.err"
        echo "running topo with $racks racks, outfile: $OUT_FILE"

        FAST=build/fast_"$racks"_racks_"$comms"_comms
        cp $FAST build/fast

        if [ "$#" -ne 1 ]; then
            sudo cset shield -e -- build/fast -c 7ffe -n 3 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE
        elif [ "$1" = "mlx" ]; then
            # run on mellanox
            sudo cset shield -e -- build/fast -c 7ffe -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > $OUT_FILE 2> $ERR_FILE
        else
            echo "unrecognized argument $1"
        fi

        grep "30 seconds" $OUT_FILE

        TPUT=`grep "30 seconds" $OUT_FILE | cut -f 6 -d ' ' | awk '{ SUM += $1} END { print SUM }'`
        echo "${racks},${comms},${i},${TPUT},${description}" >> $OUT_SUMMARY
        
        # pause to allow huge pages to become available again
        sleep 1s
    done
done

sudo cset shield --reset
