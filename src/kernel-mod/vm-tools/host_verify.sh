#!/bin/sh

# script to verify that we are set up to run the Fastpass arbiter,
# with an endpoint in a vm
# much of this is copied from setup.sh in the dpdk download


# first start the vm, using run.sh in kernel-mod/vm-tools

# check the bridge
echo "host_verify.sh: check for br0 between the vm and eth1"
sudo ifconfig br0
sudo brctl show

# check the huge pages
echo "host_verify.sh: check for huge pages"
grep -i huge /proc/meminfo

# check the IGB UIO module
echo "host_verify.sh: check for the IGB UIO module"
/sbin/lsmod | grep igb_uio

# check that eth1 is bound to the IGB UIO module
echo "host_verify.sh: check if eth1 is bound to the IGB UIO module"
${RTE_SDK}/tools/dpdk_nic_bind.py --status
