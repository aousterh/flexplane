#!/bin/bash

# run several emulators with different numbers of racks, all with 1 Core switch

n_racks=( 1 )
schemes=( DROP_TAIL RED DCTCP HULL_SCHED ROUND_ROBIN PFABRIC \
                    PRIO_BY_FLOW_QUEUEING )

# create a directory for logging
mkdir /tmp/aousterh-logs

# shield cpus appropriately
sudo cset shield -c 1-12

for scheme in "${schemes[@]}"
do
    for racks in "${n_racks[@]}"
    do
        for i in {1..5}
        do
            OUT_FILE="emu_output_${scheme}_${racks}_rack_${i}.out"
            ERR_FILE="emu_output_${scheme}_${racks}_rack_${i}.err"
            fast_name=build/fast_"$scheme"_"$racks"_racks
            echo "running topo with $racks racks, outfile: $OUT_FILE, executable: $fast_name"

            sudo cset shield -e -- $fast_name -c 1ffe -n 3 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE

            grep "30 seconds" $OUT_FILE

            # pause to allow huge pages to become available again
            sleep 1s
        done
    done
done

sudo cset shield --reset
