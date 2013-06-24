KERN_DIR = /home/joy/workspace/linux-2.6.13-hzh

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= at91_spi.o
obj-m	+= spi_dev.o
