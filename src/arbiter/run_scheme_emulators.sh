#!/bin/bash

# run several emulators with different numbers of racks, all with 1 Core switch

n_racks=( 1 )
schemes=( PFABRIC DROP_TAIL RED DCTCP HULL_SCHED ROUND_ROBIN \
                    PRIO_BY_FLOW_QUEUEING LSTF )
description="all_schemes_jan_13"

# create a directory for logging
mkdir /tmp/aousterh-logs

# shield cpus appropriately
sudo cset shield -c 1-14

mkdir $description
OUT_SUMMARY=$description/"emu_scheme_output_summary_${description}.csv"
echo "scheme,racks,index,tput,label,type" > $OUT_SUMMARY

for scheme in "${schemes[@]}"
do
    for racks in "${n_racks[@]}"
    do
        for i in {1..5}
        do
            OUT_FILE=$description/"emu_output_${scheme}_${racks}_rack_${i}.out"
            ERR_FILE=$description/"emu_output_${scheme}_${racks}_rack_${i}.err"
            fast_name=build/fast_"$scheme"_"$racks"_racks
            echo "running topo with $racks racks, outfile: $OUT_FILE, executable: $fast_name"
            cp $fast_name build/fast

            sudo cset shield -e -- build/fast -c 7ffe -n 3 -m 512 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE

            grep "30 seconds" $OUT_FILE

            TPUT=`grep "30 seconds" $OUT_FILE | cut -f 6 -d ' ' | awk '{ SUM += $1} END { print SUM }'`
            echo "${scheme},${racks},${i},${TPUT},${description},in_rack" >> $OUT_SUMMARY

            # pause to allow huge pages to become available again
            sleep 1s
        done
    done
done

sudo cset shield --reset
