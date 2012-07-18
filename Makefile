obj-m += mmc.o
mmc-objs :=parapin.o sd.o block.o
all:
	clear
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
  
clean:
	clear
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
