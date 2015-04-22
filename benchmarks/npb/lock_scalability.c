/**
 * \file
 * \brief Lock scalability benchmark.
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
#include <omp.h>
#include <stdlib.h>
#include "../include/barrelfish/threads.h"
//#include "../lib/barrelfish/threads.h"

// Use spinlockts if defined, mutexes otherwise
//#undef SPINLOCKS
//#define SPINLOCKS
//#define POSIX 1

#ifdef POSIX
#include <pthread.h>
#include <stdint.h>
#endif

#define N 1000000

#ifdef SPINLOCKS 
/** \brief spinlockt */
typedef volatile uint64_t spinlockt_t __attribute__ ((aligned(64)));

static inline void _acquire_spinlockt(spinlockt_t * volatile lock)
{
    __asm__ __volatile__(
                         "0:\n\t"
                         "xor %%rax,%%rax\n\t"
                         "lock bts %%rax,(%0)\n\t"
                         "jc 0b\n\t"
                         : : "S" (lock) : "rax"
                        );
}

static inline void _release_spinlockt(spinlockt_t * volatile lock)
{
    *lock = 0;
}
#endif

static inline uint64_t rdtsc(void)
{
    uint64_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return (edx << 32) | eax;
}

int main(int argc, char *argv[])
{
        int i=0;
	int threads = 4;
	if(argv[1])
	  threads = atoi(argv[1]);

	bomp_custom_init();
        omp_set_num_threads(threads);

#ifndef POSIX
#ifndef SPINLOCKS
        static struct thread_mutex lock = THREAD_MUTEX_INITIALIZER;
	printf("m1\n");
#else
        static spinlockt_t lock = 0;
#endif
#else
#ifdef SPINLOCKS
        static spinlockt_t lock = 0;
#else
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	printf("m2\n");
#endif
#endif

        uint64_t begin = rdtsc();

        #pragma omp parallel 
        {
#pragma omp for private(i)
		   for(i=0;i<N;i++)
		   {
#ifdef SPINLOCKS
                       _acquire_spinlockt(&lock);
                       _release_spinlockt(&lock);
#else
                       pthread_mutex_lock(&lock);
                       pthread_mutex_unlock(&lock);
#endif
		   }
	}

        uint64_t end = rdtsc();

        printf("took %lu\n", end - begin);
}

