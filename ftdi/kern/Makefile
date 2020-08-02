ifneq ($(KERNELRELEASE),)

obj-m := ftdi.o

else

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

endif
