#!/bin/bash

# check arguments
if [ "$#" -ne 1 ]; then
    echo "please specify one input argument: 'endpoint' or 'arbiter'"
    exit 0
fi

if [ "$1" = "endpoint" ]; then
    echo "restart time sync at endpoint"
    PATH_TO_CONFS=~/timesync-confs/sv
elif [ "$1" = "arbiter" ]; then
    echo "restart time sync at arbiter"
    PATH_TO_CONFS=~/timesync-confs/master.sv
else
    echo "unrecognized argument $1. please specify 'endpoint' or 'arbiter'"
    exit 0
fi

# kill existing ptp4l, phc2sys, and ntp
echo "kill any running ptp4l and phc2sys"
sudo pkill ptp4l
sudo pkill phc2sys
if [ "$1" = 'endpoint' ]; then
    # only kill ntp at endpoints
    echo "kill ntp"
    sudo pkill ntpd
fi

# restart ptp4l and phc2sys
pushd $PATH_TO_CONFS/ptp4l/
sudo ./run > $PATH_TO_CONFS/ptp4l/ptp4l.log &
cd $PATH_TO_CONFS/phc2sys/
sudo ./run > $PATH_TO_CONFS/phc2sys/phc2sys.log &
popd

# wait 10 seconds and then check the logs
echo "wait 10 seconds"
sleep 10
echo "check log output"
tail $PATH_TO_CONFS/ptp4l/ptp4l.log
tail $PATH_TO_CONFS/phc2sys/phc2sys.log