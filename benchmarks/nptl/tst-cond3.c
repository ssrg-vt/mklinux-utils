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
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<syscall.h>
#include "tst-cond.h"
/* Note that this test requires more than the standard.  It is
   required that there are no spurious wakeups if only more readers
   are added.  This is a reasonable demand.  */


static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;


static int N=4;


static void *
tf (void *arg)
{
  int i = (long int) arg;
  int err;
  int cpu =i;

    printf ("child %d: lock cpu{%d} \n",syscall(SYS_gettid),cpu+1);

    //  switch(cpu+1);
    int __ret;
    int cpu_src = sched_getcpu();
    int cpu_dest= i;
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu_dest, &cpu_mask);
    
    
     __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
  /* Get the mutex.  */
  err = pthread_mutex_lock (&mut);
  if (err != 0)
    {
      printf ("child %d mutex_lock failed: %s\n", i, strerror (err));
      exit (1);
    }

  /* This call should never return.  */
  pthread_cond_wait (&cond, &mut);

  /* We should never get here.  */
//  exit (1);

  return NULL;
}


int
cond3 (int t)
{
  if(t!=0) N=t;

  int err;
  int i;

  for (i = 0; i < N; ++i)
    {
      pthread_t th;

      if (i != 0)
	{
	  /* Release the mutex.  */
	  err = pthread_mutex_unlock (&mut);
	  if (err != 0)
	    {
	      printf ("mutex_unlock %d failed: %s\n", i, strerror (err));
	      return 1;
	    }
	}

      err = pthread_create (&th, NULL, tf, (void *) (long int) i+1);
      if (err != 0)
	{
	  printf ("create %d failed: %s\n", i, strerror (err));
	  return 1;
	}

      /* Get the mutex.  */
      err = pthread_mutex_lock (&mut);
      if (err != 0)
	{
	  printf ("mutex_lock %d failed: %s\n", i, strerror (err));
	  return 1;
	}
    }

  /* Set an alarm for 1 second.  The wrapper will expect this.  */
  alarm (1);

  /* This call should never return.  */
  pthread_cond_wait (&cond, &mut);

  puts ("cond_wait returned");
  return 1;
}


#define EXPECTED_SIGNAL SIGALRM
