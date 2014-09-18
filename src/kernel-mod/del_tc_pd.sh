#!/bin/bash

# script for removing the Fastpass kernel module from a machine in the pd cluster

DEV="em1"
TC="/home/aousterh/src/iproute2-fastpass/tc/tc"

sudo $TC qdisc del dev $DEV root
sudo rmmod fastpass
echo -- lsmod empty --
sudo lsmod | grep fastpass
echo -----------------
