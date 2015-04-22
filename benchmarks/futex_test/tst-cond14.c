/* Copyright (C) 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2004.

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
#include<syscall.h>
#include <unistd.h>

#include "tst-cond.h"
#include <stdint.h>

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mut = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t mut2 = PTHREAD_MUTEX_INITIALIZER;

static void *
tf (void *p)
{
     int cpu = (int)p;
     printf ("child %lu: lock cpu{%lu} \n",syscall(SYS_gettid),(cpu));

     int __ret;
     int cpu_src = sched_getcpu();
     int cpu_dest= (cpu+1)%4;
     cpu_set_t cpu_mask;
     CPU_ZERO(&cpu_mask);
     CPU_SET(cpu_dest, &cpu_mask);
   __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);

  if (pthread_mutex_lock (&mut) != 0)
    {
      exit (1);
    }
  if (pthread_mutex_lock (&mut) != 0)
    {
      exit (1);
    }
  if (pthread_mutex_lock (&mut) != 0)
    {
      exit (1);
    }

  if (pthread_mutex_unlock (&mut2) != 0)
    {
      exit (1);
    }

  if (pthread_cond_wait (&cond, &mut) != 0)
    {
      exit (1);
    }

  return NULL;
}


 int cond14 (int cpu)
{
  
uint64_t start = 0;
uint64_t end = 0;
start = rdtsc();
  int c = 1;
  if(cpu>0)
	c = cpu;
  if (pthread_mutex_lock (&mut2) != 0)
    {
      puts ("1st mutex_lock failed");
      return 1;
    }

  puts ("parent: create child");

  pthread_t th;
  int err = pthread_create (&th, NULL, tf,(void*)c);
  if (err != 0)
    {
      printf ("parent: cannot create thread: %s\n", strerror (err));
      return 1;
    }

  /* We have to synchronize with the child.  */
  if (pthread_mutex_lock (&mut2) != 0)
    {
      puts ("2nd mutex_lock failed");
      return 1;
    }

  /* Give the child to reach to pthread_cond_wait.  */
  sleep (1);

  if (pthread_cond_signal (&cond) != 0)
    {
      puts ("cond_signal failed");
      return 1;
    }

  err = pthread_join (th, NULL);
  if (err != 0)
    {
      printf ("parent: failed to join: %s\n", strerror (err));
      return 1;
    }

  puts ("done");

end = rdtsc();
//printf("compute time {%ld} \n",(end-start));
  return 0;
}


#define TIMEOUT 3
