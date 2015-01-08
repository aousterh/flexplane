#!/bin/bash

DEV="eth0"
TC="./tc"

sudo $TC qdisc del dev $DEV root
sudo rmmod fastpass
echo -- lsmod empty --
sudo lsmod | grep fastpass
echo -----------------
