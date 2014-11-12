#!/bin/bash

# check arguments
if [ "$#" -ne 1 ]; then
    echo "please specify one input argument: 'endpoint' or 'arbiter'"
    exit 0
fi

if [ "$1" = "endpoint" ]; then
    echo "check time sync status at endpoint"
    PATH_TO_CONFS=~/timesync-confs/sv
elif [ "$1" = "arbiter" ]; then
    echo "check time sync status at arbiter"
    PATH_TO_CONFS=~/timesync-confs/master.sv
else
    echo "unrecognized argument $1. please specify 'endpoint' or 'arbiter'"
    exit 0
fi

tail $PATH_TO_CONFS/ptp4l/ptp4l.log
tail $PATH_TO_CONFS/phc2sys/phc2sys.log