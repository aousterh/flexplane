#!/bin/bash

mkdir -p log/

if [ "$#" -ne 1 ]; then
    FAST="./fast"
    echo "running default arbiter (should be drop tail)"
elif [ "$1" = "drop_tail"  ]; then
    FAST="./fast_drop_tail"
    echo "running drop tail arbiter"
elif [ "$1" = "red"  ]; then
    FAST="./fast_red"
    echo "running red arbiter"
elif [ "$1" = "dctcp" ]; then
    FAST="./fast_dctcp"
    echo "running dctcp arbiter"
elif [ "$1" = "prio" ]; then
    FAST="./fast_prio"
    echo "running prio arbiter"
elif [ "$1" = "prio_by_flow" ]; then
    FAST="./fast_prio_by_flow"
    echo "running prio by flow arbiter"
elif [ "$1" = "rr" ]; then
    FAST="./fast_rr"
    echo "running rr arbiter"
elif [ "$1" = "hull" ]; then
    FAST="./fast_hull"
    echo "running hull arbiter"
elif [ "$1" = "hull_sched" ]; then
    FAST="./fast_hull_sched"
    echo "running hull as a scheduler arbiter"
elif [ "$1" = "pfabric" ]; then
    FAST="./fast_pfabric"
    echo "running pfabric arbiter"
else
    echo "unrecognized arbiter type $1"
    exit
fi

# clear switch logs
rm -fr ./log/queues-*.csv

sudo $FAST -c 7 -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > arbiter_log.txt 2> arbiter_error.txt
