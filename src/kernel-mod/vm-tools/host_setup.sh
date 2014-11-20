#!/bin/sh

# script to prepare to run the Fastpass arbiter, with an endpoint in a vm
# much of this is copied from setup.sh in the dpdk download


#
# Creates hugepage filesystem.
#
create_mnt_huge()
{
	echo "Creating /mnt/huge and mounting as hugetlbfs"
	sudo mkdir -p /mnt/huge

	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -ne 0 ] ; then
		sudo mount -t hugetlbfs nodev /mnt/huge
	fi
}

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
# Creates hugepages.
#
set_512_non_numa_pages()
{
	clear_huge_pages

	Pages=512

	echo "echo $Pages > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages" > .echo_tmp

	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
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
# Loads new igb_uio.ko (and uio module if needed).
#
load_igb_uio_module()
{
	if [ ! -f $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko ];then
		echo "## ERROR: Target does not have the DPDK UIO Kernel Module."
		echo "       To fix, please try to rebuild target."
		return
	fi

	remove_igb_uio_module

	/sbin/lsmod | grep -s uio > /dev/null
	if [ $? -ne 0 ] ; then
		if [ -f /lib/modules/$(uname -r)/kernel/drivers/uio/uio.ko ] ; then
			echo "Loading uio module"
			sudo /sbin/modprobe uio
		fi
	fi

	# UIO may be compiled into kernel, so it may not be an error if it can't
	# be loaded.

	echo "Loading DPDK UIO module"
	sudo /sbin/insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko
	if [ $? -ne 0 ] ; then
		echo "## ERROR: Could not load kmod/igb_uio.ko."
		echo "try rebuilding igb_uio.ko using setup.sh?"
		quit
	fi
}

#
# Uses pci_unbind.py to move devices to work with igb_uio
#
bind_eth1()
{
	if  /sbin/lsmod  | grep -q igb_uio ; then
		PCI_PATH="0000:02:00.0"
		sudo ${RTE_SDK}/tools/dpdk_nic_bind.py -b igb_uio $PCI_PATH && echo "OK"
	else 
		echo "# Please load the 'igb_uio' kernel module before querying or "
		echo "# adjusting NIC device bindings"
	fi
}


# first start the vm, using run.sh in kernel-mod/vm-tools

# setup a bridge to the vm
echo "host_setup.sh: creating a bridge between the vm and eth1"
sudo brctl addbr br0
sudo brctl addif br0 tap0
sudo brctl addif br0 eth1
sudo ifconfig br0 up 10.1.1.3

# setup huge pages
echo "host_setup.sh: setting up 512 huge pages for non-numa"
set_512_non_numa_pages

# insert the IGB UIO module
echo "host_setup.sh: inserting the IGB UIO module"
load_igb_uio_module

# bind eth1 to the IGB UIO module
echo "host_setup.sh: binding eth1 to the IGB UIO module"
bind_eth1
