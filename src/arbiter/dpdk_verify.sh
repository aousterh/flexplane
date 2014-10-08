#!/bin/sh

# script to verify that dpdk is set up to run the Fastpass arbiter


# check the huge pages
echo "dpdk_verify.sh: check for huge pages"
grep -i huge /proc/meminfo

# check the IGB UIO module
echo "dpdk_verify.sh: check for the IGB UIO module"
/sbin/lsmod | grep igb_uio

# check that eth is bound to the IGB UIO module
echo "dpdk_verify.sh: check if eth is bound to the IGB UIO module"
if [ -f ${RTE_SDK}/tools/pci_unbind.py ]; then
    sudo ${RTE_SDK}/tools/pci_unbind.py --status
elif [ -f ${RTE_SDK}/tools/dpdk_nic_bind.py ]; then
    sudo ${RTE_SDK}/tools/dpdk_nic_bind.py --status
else
    "error locating script to bind NICs in ${RTE_SDK}/tools"
fi
