#!/bin/sh

# script to teardown all dpdk state set up for the Fastpass arbiter

PCI_PATH="0000:82:00.1"
DEV="eth4"
DRV="igb"

#
# Removes hugepage filesystem.
#
remove_mnt_huge()
{
	echo "Unmounting /mnt/huge and removing directory"
	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -eq 0 ] ; then
		sudo umount /mnt/huge
	fi

	if [ -d /mnt/huge ] ; then
		sudo rm -R /mnt/huge
	fi
}

#
# Removes all reserved hugepages.
#
clear_huge_pages()
{
	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		echo "echo 0 > $d/hugepages/hugepages-2048kB/nr_hugepages" >> .echo_tmp
	done
	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Unloads igb_uio.ko.
#
remove_igb_uio_module()
{
	echo "Unloading any existing DPDK UIO module"
	/sbin/lsmod | grep -s igb_uio > /dev/null
	if [ $? -eq 0 ] ; then
		sudo /sbin/rmmod igb_uio
	fi
}

#
# Uses pci_unbind.py to move devices to work with kernel drivers again
#
unbind_eth()
{
        if [ -f ${RTE_SDK}/tools/pci_unbind.py ]; then
            sudo ${RTE_SDK}/tools/pci_unbind.py -b $DRV $PCI_PATH && echo "OK"
        elif [ -f ${RTE_SDK}/tools/dpdk_nic_bind.py ]; then
            sudo ${RTE_SDK}/tools/dpdk_nic_bind.py -b $DRV $PCI_PATH && echo "OK"
        else
            "error locating script to bind NICs in ${RTE_SDK}/tools"
        fi
}


# remove huge page mappings
echo "dpdk_teardown.sh: clearing huge pages"
clear_huge_pages

# unbind eth from the IGB UIO module
echo "dpdk_teardown.sh: unbinding eth from the IGB UIO module"
unbind_eth

# insert the IGB UIO module
echo "dpdk_teardown.sh: removing the IGB UIO module"
remove_igb_uio_module

# bring up eth again
echo "dpsk_teardown.sh: bringing up eth again"
sudo ifconfig $DEV up
