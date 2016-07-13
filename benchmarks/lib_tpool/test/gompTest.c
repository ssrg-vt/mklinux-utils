#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

/* Threading */
#include <pthread.h>
#include <omp.h>
#define NUM_THREADS 5

int counter = 0;

#pragma popcorn
void offload_func(int num_threads)
{
 int i;
  omp_set_num_threads(num_threads);
        #pragma omp parallel for
	for(i = 0; i < num_threads; i++)
        {
                #pragma omp critical
                 printf("Counter: %d\n", counter);
        }
        printf("Counter: %d\n", counter);
}

/* Basic functionality tests for libc */
int main(int argc, char** argv)
{
        // OpenMP
        int nThreads=8;
        printf("Going to migrate - this requires printf in the remote kernel\n");
        offload_func(nThreads);

        return 0;
}

