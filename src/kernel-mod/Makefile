ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := fastpass.o
#obj-m  := simple.o
fastpass-y := sch_timeslot.o sch_fastpass.o fastpass_proto.o ../protocol/fpproto.o compat-3_2.o
simple-y := sch_timeslot.o sch_simple.o


else
# normal makefile

KCPPFLAGS = -DFASTPASS_ENDPOINT
KCPPFLAGS += -DCONFIG_IP_FASTPASS_DEBUG
KCPPFLAGS += -DEMULATION_ALGO
KCPPFLAGS += -I/usr/include
#KCPPFLAGS += -DPIPELINED_ALGO

HOSTCC = gcc
CC = gcc
HOSTCC = /mnt/vol/engshare/third-party/centos6-native/gcc/gcc-4.7.1/cc6c9dc/bin/gcc
CC = /mnt/vol/engshare/third-party/centos6-native/gcc/gcc-4.7.1/cc6c9dc/bin/gcc

default:
	$(MAKE) -C $(KDIR) M=$$PWD KCPPFLAGS="$(KCPPFLAGS)" HOSTCC="$(HOSTCC)" CC="$(CC)"
clean:
	rm -f fastpass.o sch_fastpass.o sch_timeslot.o fastpass_proto.o ../protocol/fpproto.o fastpass.ko compat-3_2.o

test:
	gcc -g -O0 -o tests/window_test tests/window_test.c
	tests/window_test
endif
