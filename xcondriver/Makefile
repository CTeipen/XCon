ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m += xcon.o

all:
	make -C /usr/src/linux-headers-$(shell uname -r)/ M=$(PWD) modules

clean:
	make -C /usr/src/linux-headers-$(shell uname -r)/ M=$(PWD) clean
