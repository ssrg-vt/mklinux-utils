#CC_FLAGS=-DDEBUG

LIB = popcorn.o

all: dump_shdr rd_cmdline wr_cmdline copy_ramdisk mpart tunnel tunnel_shm kcore mpart


$(LIB): popcorn.c
	$(CC) $(CC_FLAGS) -c popcorn.c

dump_shdr: dump_shdr.c $(LIB)
	$(CC) $(CC_FLAGS) -o $@ $(LIB) dump_shdr.c

rd_cmdline: read_cmdline.c $(LIB)
	$(CC) $(CC_FLAGS) -o $@ $(LIB) read_cmdline.c

wr_cmdline: write_cmdline.c $(LIB)
	$(CC) $(CC_FLAGS) -o $@  $(LIB) write_cmdline.c

copy_ramdisk: copy_ramdisk.c $(LIB)
	$(CC) $(CC_FLAGS) -o $@ $(LIB) copy_ramdisk.c

mpart: mpart.c
	$(CC) $(CC_FLAGS) -o $@ mpart.c

tunnel: tunnel.c
	$(CC) $(CC_FLAGS) -o $@ tunnel.c -lpthread

tunnel_shm: tunnel_shm.c
	$(CC) $(CC_FLAGS) -o $@ tunnel_shm.c -lpthread

kcore: kcore.c
	$(CC) $(CC_FLAGS) -o $@ kcore.c 


.PHONY: clean

clean:
	rm -f *.o dump_shdr rd_cmdline wr_cmdline copy_ramdisk mpart tunnel tunnel_shm kcore

