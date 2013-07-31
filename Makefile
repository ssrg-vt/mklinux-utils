#CC_FLAGS=-DDEBUG

dump_shdr: popcorn.o
	$(CC) $(CC_FLAGS) -o dump_shdr popcorn.o dump_shdr.c
rd_cmdline: popcorn.o
	$(CC) $(CC_FLAGS) -o rd_cmdline popcorn.o read_cmdline.c
wr_cmdline: popcorn.o
	$(CC) $(CC_FLAGS) -o wr_cmdline  popcorn.o write_cmdline.c
copy_ramdisk: popcorn.o
	$(CC) $(CC_FLAGS) -o copy_ramdisk popcorn.o copy_ramdisk.c
mpart:

tunnel:

kcore:

all: dump_shdr rd_cmdline wr_cmdline copy_ramdisk

.PHONY: clean

clean:
	rm *.o dump_shdr rd_cmdline wr_cmdline copy_ramdisk
#mpart tunnel kcore
