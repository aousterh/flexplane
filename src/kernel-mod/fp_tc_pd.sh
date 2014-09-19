#!/bin/bash

# script for inserting the Fastpass kernel module on a machine in the pd cluster

CONTROLLER_IP="18.26.4.181"
DEV="em1"

TC="/home/aousterh/src/iproute2-fastpass/tc/tc"

./del_tc_pd.sh

sudo insmod fastpass.ko fastpass_debug=1 req_cost=1000000 req_bucketlen=1000000 ctrl_addr=$CONTROLLER_IP update_timer_ns=2048000 retrans_timeout_ns=3000000

echo -- lsmod after --
lsmod | grep fastpass
echo -----------------

TEXT=`cat /sys/module/fastpass/sections/.text`
DATA=`cat /sys/module/fastpass/sections/.data`
BSS=`cat /sys/module/fastpass/sections/.bss`
echo add-symbol-file $FASTPASS_DIR/src/kernel-mod/fastpass.ko $TEXT -s .data $DATA -s .bss $BSS

sudo $TC qdisc add dev $DEV root fastpass rate 12500Kbps
