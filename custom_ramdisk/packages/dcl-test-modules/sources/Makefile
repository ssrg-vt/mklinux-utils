obj-m := mem_access.o vmalloc2.o page_array.o module_area.o
KDIR := /root/mklinux

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install: default
	scp -r README.txt *.ko $(TARGET)

clean:	
	rm -f *.o *.ko *~ *.symvers *.order *.unsigned *.mod.c
