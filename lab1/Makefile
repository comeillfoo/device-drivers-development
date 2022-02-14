NAME=chr_comp
DEV_NAME=var2
DEV_MAJOR=410
DEV_MINOR=0
MAKE=make
KDIR?=/lib/modules/$(shell uname -r)/build
# specify your number of logic cores
nproc?=8

obj-m += $(NAME).o
$(NAME)-objs := src/_$(NAME).o # add here additional object files for compiling

all:
	$(MAKE) -j $(nproc) -C $(KDIR) M=$(shell pwd) modules

load:
	insmod $(NAME).ko

unload:
	rmmod  $(NAME) -f

mkdev:
	sudo mknod /dev/$(DEV_NAME) c $(DEV_MAJOR) $(DEV_MINOR) && sudo chmod a+rw /dev/$(DEV_NAME)

rmdev:
	sudo rm -f /dev/$(DEV_NAME)

clean:
	$(MAKE) -j 8 -C $(KDIR) M=$(shell pwd) clean