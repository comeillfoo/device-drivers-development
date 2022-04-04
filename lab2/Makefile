NAME=vblkdev
MAKE=make
KDIR?=/lib/modules/$(shell uname -r)/build
# specify your number of logic cores
nproc?=8

obj-m += $(NAME).o
$(NAME)-objs := src/_$(NAME).o # add here additional object files for compiling

all:
	make -j $(nproc) -C $(KDIR) M=$(shell pwd) modules

clean:
	make -j $(nproc) -C $(KDIR) M=$(shell pwd) clean

do:
	sudo insmod $(NAME).ko

rm:
	sudo rmmod $(NAME).ko
