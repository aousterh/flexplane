#!/bin/bash

# run several emulators with different numbers of racks, all with 1 Core switch
# first arg is "mlx" if we should compile for mellanox, empty otherwise

racks=1
#schemes=( DROP_TAIL RED DCTCP HULL_SCHED ROUND_ROBIN PFABRIC PRIO_BY_FLOW_QUEUEING )
schemes=( PFABRIC )

# shield cpus appropriately
sudo cset shield -c 1-7

for scheme in "${schemes[@]}"
do
    for i in {1..5}
    do
        OUT_FILE="emu_output_${scheme}_1_rack_${i}_faster_112.out"
        ERR_FILE="emu_output_${scheme}_1_rack_${i}_faster_112.err"
        echo "running topo with $racks racks, outfile: $OUT_FILE"

        if [ "$#" -ne 1 ]; then
            sudo cset shield -e -- build/fast_"$scheme"_1_rack -c fe -n 3 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE
#        sudo build/fast_"$racks"_racks -c fe -n 3 --no-hpet -- -p 1 > $OUT_FILE 2> $ERR_FILE
        elif [ "$1" = "mlx" ]; then
            # run on mellanox
            sudo cset shield -e -- build/fast_"$racks"_racks -c fe -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > $OUT_FILE 2> $ERR_FILE
#        sudo build/fast_"$racks"_racks -c fe -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > $OUT_FILE 2> $ERR_FILE
        else
            echo "unrecognized argument $1"
        fi

        grep "30 seconds" $OUT_FILE

        # pause to allow huge pages to become available again
        sleep 1s
    done
done

sudo cset shield --reset
