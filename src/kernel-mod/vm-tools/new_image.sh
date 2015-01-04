#!/bin/sh

. ~/fastpass_env.sh

sudo qemu-system-x86_64 -m 1G \
    -enable-kvm -cpu host \
    -netdev user,id=mynet0,net=10.1.1.0/24 \
    -device virtio-net,netdev=mynet0 \
    -drive file=$VM_HD_IMAGE,if=virtio \
     -cdrom ~/Downloads/CentOS-7.0-1406-x86_64-Minimal.iso \
     -boot d -smp 8

#    -hda $VM_HD_IMAGE \
