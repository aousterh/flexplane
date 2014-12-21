To run the arbiter stress test:
(1) get the source code. in the same directory:
    git clone git@bitbucket.org:yonch/fastpass-private.git
    git clone git@bitbucket.org:yonch/fastpass-public.git
(2) get DPDK, version 1.7.1 (latest version as of October 2014)
    curl http://dpdk.org/browse/dpdk/snapshot/dpdk-1.7.1.tar.gz | tar -xvz
(3) setup DPDK
    export RTE_SDK=’pwd’/dpdk-1.7.1
    export RTE_TARGET=x86_64-native-linuxapp-gcc
    may need to change CONFIG_RTE_MAX_MEMSEG in
    	config/defconfig_x86_64-native-linuxapp-gcc to 512
    copy rte.compile-pre.mk from src/arbiter to /dpdk-1.7.1/mk/internal/
    	 (overwrite the version there)
(4) build DPDK
    in dpdk-1.7.1/tools, ./setup.sh, choose the option for
       'x86_64-native-linuxapp-gcc' in Step 1
(5) setup huge pages, etc. for DPDK
    in fastpass-public/src/arbiter, run ./dpdk_setup.sh
(6) set arbiter to use stress test
    in fastpass-public/src/arbiter, set IS_STRESS_TEST in control.h to 1
(7) build and run arbiter
    in fastpass-public/src/arbiter, make clean && make -j5
    ./run.sh


when done:
     in fastpass-public/src/arbiter, run ./dpdk_teardown.sh
