/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Test of POSIX barriers.  */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include "tst-barrier.h"
#include <signal.h>
//#include "switch_cpu.h"
#include<stdint.h>

#define  NTHREADS 2

static int ROUNDS= 20;

static pthread_barrier_t barriers[NTHREADS];

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int counters[NTHREADS];
static int serial[NTHREADS];

void sig_handler(int signo)
{
  if (signo == SIGSEGV)
    printf("received SIGINT\n");
}
static inline int switch_cpu2(int cpuid)
{
	int __ret;
	int cpu_src = sched_getcpu();
	int cpu_dest=1;

	if(cpuid != cpu_src)
		     cpu_dest = cpuid;
	else 
		return 0;

	cpu_set_t cpu_mask;
	CPU_ZERO(&cpu_mask);
	CPU_SET(cpu_dest, &cpu_mask);


	 __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);


	  return __ret;

}

static void *
worker (void *arg)
{
  void *result = NULL;
  int nr = (long int) arg;
  int i;

   printf ("child %d: lock cpu{%lu} \n",syscall(SYS_gettid),nr);

  if (signal(SIGSEGV, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");


  switch_cpu2(nr);

  for (i = 0; i < ROUNDS; ++i)
    {
      int j;
      int retval;

      if (nr == 0)
	{
	  memset (counters, '\0', sizeof (counters));
	  memset (serial, '\0', sizeof (serial));
	}

      retval = pthread_barrier_wait (&barriers[NTHREADS - 1]);
      if (retval != 0 && retval != PTHREAD_BARRIER_SERIAL_THREAD)
	{
	  printf ("thread %d failed to wait for all the others\n", nr);
	  result = (void *) 1;
	}

      for (j = nr; j < NTHREADS; ++j)
	{
	  /* Increment the counter for this round.  */
	  pthread_mutex_lock (&lock);
	  ++counters[j];
	  pthread_mutex_unlock (&lock);

	  /* Wait for the rest.  */
	  retval = pthread_barrier_wait (&barriers[j]);

	  /* Test the result.  */
	  if (nr == 0 && counters[j] != j + 1)
	    {
	      result = (void *) 1;
	    }

	  if (retval != 0)
	    {
	      if (retval != PTHREAD_BARRIER_SERIAL_THREAD)
		{
		  result = (void *) 1;
		}
	      else
		{
		  pthread_mutex_lock (&lock);
		  ++serial[j];
		  pthread_mutex_unlock (&lock);
		}
	    }

	  /* Wait for the rest again.  */
	   pthread_barrier_wait (&barriers[j]);

	  /* Now we can check whether exactly one thread was serializing.  */
	  if (nr == 0 && serial[j] != 1)
	    {
	      result = (void *) 1;
	    }
	}
    }

  return result;
}


#define TIMEOUT 60
int
barrier3 (int thr, int rd)
{
uint64_t start = 0;
uint64_t end = 0;
start = rdtsc();
// if(thr > 0) NTHREADS = thr;

 if(rd > 0) ROUNDS =rd;
  
 pthread_t threads[NTHREADS];
// barriers = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t) * NTHREADS);
// counters = (int *)malloc(sizeof(int) * NTHREADS);
// serial = (int *)malloc(sizeof(int) * NTHREADS);

  int i;
  void *res;
  int result = 0;

   printf ("&lock = %p  \n\n",&lock);

  /* Initialized the barrier variables.  */
  for (i = 0; i < NTHREADS; ++i)
    if (pthread_barrier_init (&barriers[i], NULL, i + 1) != 0)
      {
	printf ("Failed to initialize barrier %d\n", i);
	exit (1);
      }
printf("&bar =%p \n",barriers);
  /* Start the threads.  */
  for (i = 0; i < NTHREADS; ++i)
    if (pthread_create (&threads[i], NULL, worker, (void *) (long int) i) != 0)
      {
	printf ("Failed to start thread %d\n", i);
	exit (1);
      }

  /* And wait for them.  */
  for (i = 0; i < NTHREADS; ++i)
    if (pthread_join (threads[i], &res) != 0 || res != NULL)
      {
	printf ("thread %d returned a failure\n", i);
	result = 1;
      }
    else
      printf ("joined threads %d\n", i);

  if (result == 0)
    puts ("all OK");

end = rdtsc();
//printf("compute time {%ld} \n",(end-start));
  return result;
}

