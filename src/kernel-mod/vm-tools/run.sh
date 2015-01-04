#!/bin/sh

. ~/fastpass_env.sh

./mount_vm.sh
sleep 0.5

sudo cp $FASTPASS_DIR/src/kernel-mod/fastpass.ko $VM_HD_MOUNTPOINT/root
sudo cp $FASTPASS_DIR/src/kernel-mod/vm-tools/* $VM_HD_MOUNTPOINT/root
sudo rm $VM_HD_MOUNTPOINT/sbin/tc
sudo rm $VM_HD_MOUNTPOINT/root/tc
sudo cp ~/src/iproute2-fastpass/tc/tc $VM_HD_MOUNTPOINT/sbin
sudo cp $FASTPASS_DIR/src/kernel-mod/vm-tools/interfaces $VM_HD_MOUNTPOINT/etc/network/interfaces

#sudo cat /dev/null > $VM_HD_MOUNTPOINT/var/log/syslog
#sudo cat /dev/null > $VM_HD_MOUNTPOINT/var/log/kern.log
#sudo cat /dev/null > $VM_HD_MOUNTPOINT/var/log/debug

sync;

./umount_vm.sh
sync;
sleep 0.5

sudo qemu-system-x86_64 -m 1G \
    -enable-kvm -cpu host \
    -drive file=$VM_HD_IMAGE,if=virtio \
    -netdev tap,id=mynet0,ifname=tap0 \
    -device virtio-net,netdev=mynet0 \
    -netdev tap,id=mynet1,ifname=tap1 \
    -device virtio-net,netdev=mynet1 \
    -s -nographic \
    -kernel $KBUILD_OUTPUT/arch/x86_64/boot/bzImage \
    -append "root=/dev/vda1 console=ttyS0,115200n8 fastpass.fastpass_debug=1" 
#    -netdev user,id=mynet0,net=10.1.1.0/24 \
#qemu-system-x86_64 -m 1G -hda linux.img  -kernel arch/x86_64/boot/bzImage -append "root=/dev/sda1 console=ttyS0" -s -serial stdio -vga std -chardev vc,id=a,width=1024,height=960,cols=140,rows=50
