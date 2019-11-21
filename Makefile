# Check $ARCH and $CROSS_COMPILE before running make
# CROSS_COMPILE should not be set for native compiler
#

ifneq ($(KERNELRELEASE),)

obj-m := goonj.o

else
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
	# make clean
clean:
	-rm *.mod.c *.o .*.cmd Module.symvers modules.order 2>/dev/null || :
	-rm -rf .tmp_versions 2>/dev/null || :

.PHONY: clean

/run/udev/rules.d/99-goonj.rules: 99-goonj.rules
	test -d ${@D} || mkdir -p ${@D} || :
	cp $? $@

loadit: goonj.ko /run/udev/rules.d/99-goonj.rules
	insmod goonj.ko
	-echo /dev/goonj? /dev/led/gp? /sys/devices/virtual/misc/goonj?
	-ls -l /dev/goonj? /dev/led/gp? /sys/devices/virtual/misc/goonj?
	seq 1 100 >/dev/goonj?
	seq 1 99 | diff - /dev/goonj?

unloadit:
	-rmmod goonj
	-rm /run/udev/rules.d/99-goonj.rules

.PHONY: loadit unloadit

change add remove : /sys/devices/virtual/misc/goonj?/uevent
	echo $@ >$<

.PHONY: change add remove

endif
