CC_FLAGS=-DDEBUG

shdr: popcorn.o shdr.o

rd_cmdline: popcon.o read_cmdline.o

wr_cmdline: popcon.o write_cmdline.o 

copy_ramdisk: popcorn.o copy_ramdisk.o 

mpart:

tunnel:

kcore:

all: shdr rd_cmdline wr_cmdline copy_ramdisk mpart tunnel kcore