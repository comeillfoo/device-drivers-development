NAME=chr_comp
MAKE=make
KDIR?=/lib/modules/$(shell uname -r)/build
DEV_BASE?=var2_
NR_DEVICES?=1
# specify your number of logic cores
nproc?=8

obj-m += $(NAME).o
$(NAME)-objs := src/_$(NAME).o # add here additional object files for compiling

all:
	$(MAKE) -j $(nproc) -C $(KDIR) M=$(shell pwd) modules

load:
	insmod $(NAME).ko nr_devices=$(NR_DEVICES)

unload:
	rmmod  $(NAME) -f
	
rwaccess:
	chmod a+rw /dev/$(DEV_BASE)*

clean:
	$(MAKE) -j 8 -C $(KDIR) M=$(shell pwd) clean