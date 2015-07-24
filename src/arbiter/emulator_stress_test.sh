#!/bin/bash

# this script runs the emulator stress test with the specified arguments. it
# must be build already before you call this script. it also shields CPUs.

# check arguments
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <num_cores>"
    exit 0
fi

NUM_CORES="$1"
if [ "$NUM_CORES" -eq 3 ]; then
    CPU_MASK=e
elif [ "$NUM_CORES" -eq 4 ]; then
    CPU_MASK=1e
elif [ "$NUM_CORES" -eq 5 ]; then
    CPU_MASK=3e
else
    echo "num cores $NUM_CORES not supported"
    exit 0
fi

# prepare output file
OUT_FILE="output_emulator_stress_test_$NUM_CORES.txt"
rm $OUT_FILE

# shield cpus appropriately
sudo cset shield -c 1-$NUM_CORES

# run 5 times
for i in {0..4}
do
    echo "stress test run number $i"
    sudo cset shield -e -- ./run.sh $CPU_MASK >> $OUT_FILE
done

grep "30 seconds" $OUT_FILE

# unshield!
sudo cset shield --reset
