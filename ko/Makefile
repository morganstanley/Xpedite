################################################################################################
#
# Xpedite Loadable kernel module
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

MODULENAME=xpedite

obj-m += $(MODULENAME).o

ccflags-y := -Wno-declaration-after-statement -Werror -O3

EXTRA_CFLAGS := -I$(src)/include

$(MODULENAME)-objs := ./lib/xpedite/pmu/EventSet.o ./lib/xpedite/pmu/Formatter.o ./lib/xpedite/pmu/PMUArch.o ./lib/xpedite/pmu/PCECtl.o ./lib/xpedite/pmu/PMUDevice.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod $(MODULENAME).ko
	sudo chmod 666 /dev/$(MODULENAME)

uninstall:
	sudo rmmod $(MODULENAME)
