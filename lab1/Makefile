NAME=chr_comp
MAKE=make
nproc?=8
KDIR?=/lib/modules/$(shell uname -r)/build

obj-m += $(NAME).o
$(NAME)-objs := src/_$(NAME).o

all:
	$(MAKE) -j $(nproc) -C $(KDIR) M=$(shell pwd) modules

load:
	insmod $(NAME).ko

unload:
	rmmod  $(NAME) -f

clean:
	$(MAKE) -j 8 -C $(KDIR) M=$(shell pwd) clean