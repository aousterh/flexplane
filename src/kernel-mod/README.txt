To build and run the endpoint kernel module, using the running kernel:
(1) Build the kernel module:
    export KDIR=/lib/modules/<current kernel>/build
    export FASTPASS_DIR=<path_to_flexplane_root>
    in src/kernel-mod: make clean && make -j5
(2) Download and build the tc utility - this is the utility that inserts and
    removes the kernel module:
    git clone http://github.com/yonch/iproute2-fastpass
    sudo apt-get install flex bison iptables-dev libdb6.0-dev
    make -j22 (might have errors/warnings but as long as tc/tc builds, it’s ok)
(3) Install the kernel module on the endpoints. The script
    src/kernel-mod/fp_tc_pd.sh shows an example of how to do this.
