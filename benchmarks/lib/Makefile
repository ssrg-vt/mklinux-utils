##########################################################################
# Copyright (c) 2012, 2013, 2014, 2015, SSRG Virginia Tech
# Copyright (c) 2007, 2008, 2009, ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

CC = clang
CFLAGS = -O0 -g -gdwarf-aranges -DPOSIX -fno-stack-protector -ffunction-sections -fdata-sections

OPT = opt
OPT_FLAGS = -load /usr/local/popcorn/lib/libStackInfo.so -stack-info

# x86-64
CFLAGS_X86_64 = $(CFLAGS) -target x86_64-linux-gnu -m64 -D__X86

# aarch64
CFLAGS_AARCH64 = $(CFLAGS) -target aarch64-linux-gnu -isystem /usr/local/popcorn/aarch64/include -D__AARCH64

#all: libbomp.a libpomp.a
all: libbomp_x86.a libbomp_aRM.a

#libbomp.a: linux_backend.o omp.o parallel.o processing.o self_info.o
#	
#cthread.o:
#	$(CC) $(CFLAGS) -c cthread.c $(OTHERS)
libpomp.a: cthread.o popcorn_backend.o omp.o parallel.o processing.o self_info.o
#	$(AR) -q $@ $^

%.o: %.c
	$(CC) $(CFLAGS_X86_64) -c -emit-llvm -o $@.bc $<
	$(OPT) $(OPT_FLAGS) $@.bc -o $@.bc
	$(CC) $(CFLAGS_X86_64) -c -o $@ $@.bc >> libbomp_stack_x86_64.txt
	rm $@.bc

libbomp_x86.a: linux_backend.o omp.o parallel.o processing.o self_info.o kmp.o
	$(AR) -q $@ $^

%_arm.o: %.c
	$(CC) $(CFLAGS_AARCH64) -c -emit-llvm -o $@.bc $<
	$(OPT) $(OPT_FLAGS) $@.bc -o $@.bc
	$(CC) $(CFLAGS_AARCH64) -c -o $@ $@.bc >> libbomp_stack_aarch64.txt
	rm $@.bc

libbomp_aRM.a: linux_backend_arm.o omp_arm.o parallel_arm.o processing_arm.o self_info_arm.o kmp_arm.o
	aarch64-linux-gnu-ar -q $@ $^

tests: cthread.o
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-cthread tst-cthread.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-cthread1 tst-cthread1.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-cthread2 tst-cthread2.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-cthread3 tst-cthread3.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-cthread4 tst-cthread4.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-oddstacklimit tst-oddstacklimit.c $^
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o test-synchmalloc tst-synchmalloc.c $^

clean:
	rm -f *.o libbomp_aRM.a libbomp_x86.a libpomp.a test-*
