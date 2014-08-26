#!/bin/sh

# script to teardown all state set up for the Fastpass arbiter
# much of this is copied from setup.sh in the dpdk download


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
unbind_eth1()
{
	PCI_PATH="0000:02:00.0"
	DRV="igb"
	sudo ${RTE_SDK}/tools/pci_unbind.py -b $DRV $PCI_PATH && echo "OK"
}


# remove bridge to the vm
echo "host_teardown.sh: removing br0 between the vm and eth1"
sudo ifconfig br0 down
sudo brctl delbr br0

# setup huge pages
echo "host_teardown.sh: clearing huge pages"
clear_huge_pages

# unbind eth1 from the IGB UIO module
echo "host_teardown.sh: unbinding eth1 from the IGB UIO module"
unbind_eth1

# insert the IGB UIO module
echo "host_teardown.sh: removing the IGB UIO module"
remove_igb_uio_module
