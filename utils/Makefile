# Makefile

obj-m += cr_reader.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	cc -o reader reader.c 

clean:
	make -C $(KDIR) M=$(PWD) clean
