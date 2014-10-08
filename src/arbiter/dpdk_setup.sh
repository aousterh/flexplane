#!/bin/sh

# script to prepare DPDK to run the Fastpass arbiter

NUM_PAGES=512
PCI_PATH="0000:84:00.1"

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
set_non_numa_pages()
{
	clear_huge_pages

	Pages=$NUM_PAGES

	echo "echo $Pages > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages" > .echo_tmp

	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Creates hugepages on specific NUMA nodes.
#
set_numa_pages()
{
	clear_huge_pages

	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		node=$(basename $d)
                if [ "$node" = "node0" ]; then
                    Pages=$NUM_PAGES
                else
                    Pages=0
                fi
		echo "echo $Pages > $d/hugepages/hugepages-2048kB/nr_hugepages" >> .echo_tmp
	done
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
                echo "# Also check that RTE_SDK/RTE_TARGET are set properly"
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
                echo "# Also check that RTE_SDK/RTE_TARGET are set properly"
		quit
	fi
}

#
# Uses pci_unbind.py to move devices to work with igb_uio
#
bind_eth()
{
	if  /sbin/lsmod  | grep -q igb_uio ; then
            if [ -f ${RTE_SDK}/tools/pci_unbind.py ]; then
		sudo ${RTE_SDK}/tools/pci_unbind.py --force -b igb_uio $PCI_PATH && echo "OK"
            elif [ -f ${RTE_SDK}/tools/dpdk_nic_bind.py ]; then
		sudo ${RTE_SDK}/tools/dpdk_nic_bind.py -b igb_uio $PCI_PATH && echo "OK"
            else
                "error locating script to bind NICs in ${RTE_SDK}/tools"
            fi
	else 
		echo "# Please load the 'igb_uio' kernel module before querying or "
		echo "# adjusting NIC device bindings"
                echo "# Also check that RTE_SDK is set properly (e.g. in .profile)"
	fi
}

# setup huge pages
echo "dpdk_setup.sh: setting up 1024 huge pages for numa"
set_numa_pages

# insert the IGB UIO module
echo "dpdk_setup.sh: inserting the IGB UIO module"
load_igb_uio_module

# bind eth to the IGB UIO module
echo "dpdk_setup.sh: binding eth to the IGB UIO module"
bind_eth
