#!/bin/bash

# script for inserting the Fastpass kernel module on a machine in the pd cluster

CONTROLLER_IP="10.1.1.10"
DEV="em2"

TC="/home/aousterh/src/iproute2-fastpass/tc/tc"

# disable generic and tcp segmentation offloads
sudo ethtool -K $DEV gso off tso off

# disable TCP small queues
echo 1310720 > /proc/sys/net/ipv4/tcp_limit_output_bytes
echo 2129920 > /proc/sys/net/core/wmem_default

./del_tc_pd.sh

sudo insmod fastpass.ko fastpass_debug=1 ctrl_addr=$CONTROLLER_IP req_cost=200000 req_bucketlen=400000 retrans_timeout_ns=2000000 req_min_gap=1000

echo -- lsmod after --
lsmod | grep fastpass
echo -----------------

TEXT=`cat /sys/module/fastpass/sections/.text`
DATA=`cat /sys/module/fastpass/sections/.data`
BSS=`cat /sys/module/fastpass/sections/.bss`
echo add-symbol-file $FASTPASS_DIR/src/kernel-mod/fastpass.ko $TEXT -s .data $DATA -s .bss $BSS

sudo $TC qdisc add dev $DEV root fastpass rate 990Mbit timeslot_mul 337 timeslot_shift 22
