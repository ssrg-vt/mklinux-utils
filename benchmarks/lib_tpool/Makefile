##########################################################################
# Copyright (c) 2016, SSRG Virginia Tech
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
##########################################################################

CC = /usr/local/popcorn/bin/clang
CFLAGS = -O0 -g -gdwarf-aranges -DPOSIX -fno-stack-protector -ffunction-sections -fdata-sections

OPT = /usr/local/popcorn/bin/opt
OPT_FLAGS = -load /usr/local/popcorn/lib/libStackInfo.so -stack-info

# x86-64
CFLAGS_X86_64 = $(CFLAGS) -target x86_64-linux-gnu -m64 -D__X86 -D_DEBUG

# aarch64
CFLAGS_AARCH64 = $(CFLAGS) -target aarch64-linux-gnu -isystem /usr/local/popcorn/aarch64/include -D__AARCH64

#all: libbomp.a libpomp.a
all: libbomp_x86.a libbomp_aRM.a

%.o: %.c
	$(CC) $(CFLAGS_X86_64) -c -emit-llvm -o $@.bc $<
	$(OPT) $(OPT_FLAGS) $@.bc -o $@.bc
	$(CC) $(CFLAGS_X86_64) -c -o $@ $@.bc >> libbomp_stack_x86_64.txt
	#rm $@.bc

libbomp_x86.a: popcorn_threadpool.o omp.o parallel.o processing.o self_info.o kmp.o
	$(AR) -q $@ $^

%_arm.o: %.c
	$(CC) $(CFLAGS_AARCH64) -c -emit-llvm -o $@.bc $<
	$(OPT) $(OPT_FLAGS) $@.bc -o $@.bc
	$(CC) $(CFLAGS_AARCH64) -c -o $@ $@.bc >> libbomp_stack_aarch64.txt
	rm $@.bc

libbomp_aRM.a: popcorn_threadpool_arm.o omp_arm.o parallel_arm.o processing_arm.o self_info_arm.o kmp_arm.o
	aarch64-linux-gnu-ar -q $@ $^

clean:
	rm -f *.o libbomp_aRM.a libbomp_x86.a *.txt *.o.bc
