/**
 * \file
 * \brief libbomp test.
 */

/*
 * Copyright (c) 2007, 2008, 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <omp.h>

//#ifdef POSIX
static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}
//#endif

//#define LARGE
#undef LARGE
#ifndef LARGE
# define N 10000000
#else
# define N 100000000
#endif
static int a[N];

int main(int argc, char *argv[])
{
        uint64_t begin, end;
        int cpus, sum =0;
#ifndef LARGE	
	long int i;
#else
	int i;
#endif

#ifndef POSIX
	bomp_custom_init();
#endif
        assert(argc == 2);
        omp_set_num_threads( cpus=atoi(argv[1]) );

        for (i=0; i<N; i++) {
	  a[i] = i;
	}

        begin = rdtsc();

#pragma omp parallel for reduction( + : sum)
        for (i=0; i<N; i++) {
	  sum = sum + a[i];
	}

	end = rdtsc();

	printf("Value of sum is %d\n", sum);
	printf("Computetime %d %lu\n", cpus, (unsigned long)(end - begin));
	
#ifndef POSIX
	bomp_custom_exit();
#endif
}
