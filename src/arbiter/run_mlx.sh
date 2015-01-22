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
else
    echo "unrecognized arbiter type $1"
    exit
fi

sudo $FAST -c 7 -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1 > arbiter_log.txt
