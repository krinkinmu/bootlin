ifneq ($(KERNELRELEASE),)

obj-m := version.o

else

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

endif
