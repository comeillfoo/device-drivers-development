NAME=virt_net_if
MAKE=make
KDIR?=/lib/modules/$(shell uname -r)/build
# specify your number of logic cores
nproc?=8

obj-m += $(NAME).o
$(NAME)-objs := src/_$(NAME).o # add here additional object files for compiling

all :
	make -C $(KDIR) M=$(shell pwd) modules
clean :
	make -C $(KDIR) M=$(shell pwd) clean
