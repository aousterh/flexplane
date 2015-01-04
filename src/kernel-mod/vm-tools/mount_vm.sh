#!/bin/bash

. ~/fastpass_env.sh

#sudo kpartx -av $VM_HD_IMAGE
sudo mkdir -p $VM_HD_MOUNTPOINT
#sudo mount /dev/mapper/loop0p1 $VM_HD_MOUNTPOINT
sudo modprobe nbd max_part=63
sudo qemu-nbd -c /dev/nbd0 $VM_HD_IMAGE
sudo mount /dev/nbd0p1 $VM_HD_MOUNTPOINT
