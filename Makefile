# Check $ARCH and $CROSS_COMPILE before running make
# CROSS_COMPILE should not be set for native compiler
#

ifneq ($(KERNELRELEASE),)

obj-m := hdmi_rpi.o

else
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
	make clean
	-cp ./scripts/at.sh /home/pi/
	-cp 97-hdmi.rules /etc/udev/rules.d/
	-udevadm control --reload
clean:
	-rm *.mod.c *.o .*.cmd Module.symvers modules.order 2>/dev/null || :
	-rm -rf .tmp_versions 2>/dev/null || :

.PHONY: clean

loadit:
	-insmod hdmi_rpi.ko

unloadit:
	-rmmod hdmi_rpi
	-rm /etc/udev/rules.d/97-hdmi.rules

.PHONY: loadit unloadit

endif
