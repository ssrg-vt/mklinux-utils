/* Copyright (C) 2002, 2003, 2006 Free Software Foundation, Inc.
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


#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include<syscall.h>
#include "tst-mutex.h"

static pthread_mutex_t m;
static pthread_barrier_t b;


static void *
tf (void *arg)
{

 int nr=(int) arg;

  printf ("child %d: lock cpu{%lu} \n",syscall(SYS_gettid),(nr));
   	int __ret;
	int cpu_src = sched_getcpu();
	int cpu_dest=1;

	if(nr != cpu_src)
	  cpu_dest = nr;

	cpu_set_t cpu_mask;
	CPU_ZERO(&cpu_mask);
	CPU_SET(cpu_dest, &cpu_mask);


	 __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);

  int e = pthread_mutex_unlock (&m);
  if (e == 0)
    {
      //puts ("1st mutex_unlock in child succeeded");
      exit (1);
    }
  if (e != EPERM)
    {
      puts ("1st mutex_unlock in child didn't return EPERM");
      exit (1);
    }

  e = pthread_mutex_trylock (&m);
  if (e == 0)
    {
      //puts ("mutex_trylock in second thread succeeded");
      exit (1);
    }
  if (e != EBUSY)
    {
      //puts ("mutex_trylock returned wrong value");
      exit (1);
    }

  e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("barrier_wait failed");
      exit (1);
    }

  e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("barrier_wait failed");
      exit (1);
    }

  e = pthread_mutex_unlock (&m);
  if (e == 0)
    {
      //puts ("2nd mutex_unlock in child succeeded");
      exit (1);
    }
  if (e != EPERM)
    {
      puts ("2nd mutex_unlock in child didn't return EPERM");
      exit (1);
    }

  if (pthread_mutex_trylock (&m) != 0)
    {
      puts ("2nd mutex_trylock in second thread failed");
      exit (1);
    }

  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("3rd mutex_unlock in second thread failed");
      exit (1);
    }

  return NULL;
}


int
mutex3 (int cp)
{
  int cpu =1;

  if(cp > 0) cpu = cp;
  
  pthread_mutexattr_t a;

  if (pthread_mutexattr_init (&a) != 0)
    {
      puts ("mutexattr_init failed");
      return 1;
    }
 printf("m{%p} bar {%p}\n",&m,&b);
  if (pthread_mutexattr_settype (&a, PTHREAD_MUTEX_RECURSIVE) != 0)
    {
      puts ("mutexattr_settype failed");
      return 1;
    }

#ifdef ENABLE_PI
  if (pthread_mutexattr_setprotocol (&a, PTHREAD_PRIO_INHERIT) != 0)
    {
      puts ("pthread_mutexattr_setprotocol failed");
      return 1;
    }
#endif

  int e;
  e = pthread_mutex_init (&m, &a);
  if (e != 0)
    {
#ifdef ENABLE_PI
      if (e == ENOTSUP)
	{
	  puts ("PI mutexes unsupported");
	  return 0;
	}
#endif
      puts ("mutex_init failed");
      return 1;
    }

  if (pthread_barrier_init (&b, NULL, 2) != 0)
    {
      puts ("barrier_init failed");
      return 1;
    }

  if (pthread_mutex_lock (&m) != 0)
    {
      puts ("mutex_lock failed");
      return 1;
    }

  if (pthread_mutex_lock (&m) != 0)
    {
      puts ("2nd mutex_lock failed");
      return 1;
    }

  if (pthread_mutex_trylock (&m) != 0)
    {
      puts ("1st trylock failed");
      return 1;
    }

  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("mutex_unlock failed");
      return 1;
    }

  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("2nd mutex_unlock failed");
      return 1;
    }

  pthread_t th;
  if (pthread_create (&th, NULL, tf, (void*) cpu) != 0)
    {
      puts ("create failed");
      return 1;
    }

  e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("barrier_wait failed");
      return 1;
    }

  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("3rd mutex_unlock failed");
      return 1;
    }

  e = pthread_mutex_unlock (&m);
  if (e == 0)
    {
      puts ("4th mutex_unlock succeeded");
      return 1;
    }
  if (e != EPERM)
    {
      puts ("4th mutex_unlock didn't return EPERM");
      return 1;
    }

  e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("barrier_wait failed");
      return 1;
    }

  if (pthread_join (th, NULL) != 0)
    {
      puts ("join failed");
      return 1;
    }

  if (pthread_barrier_destroy (&b) != 0)
    {
      puts ("barrier_destroy failed");
      return 1;
    }

  if (pthread_mutex_destroy (&m) != 0)
    {
      puts ("mutex_destroy failed");
      return 1;
    }

  if (pthread_mutexattr_destroy (&a) != 0)
    {
      puts ("mutexattr_destroy failed");
      return 1;
    }

  return 0;
}

