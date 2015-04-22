/* Copyright (C) 2002, 2004, 2006 Free Software Foundation, Inc.
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
#include <time.h>
#include <sched.h>
#include<syscall.h>
#include "tst-mutex.h"
#include <stdint.h>
#ifndef TYPE
# define TYPE PTHREAD_MUTEX_DEFAULT
#endif

static int i=0;
#define forsleep(void) \
	for(i=0;i<99;i++){ \
	} 
static pthread_mutex_t lock;


static int  ROUNDS = 100;
static int N =  7;
static volatile int kounter =0;

static void *
tf (void *arg)
{
  int nr = (long int) arg;
  int cnt;

  int __ret;
  int cpu_src = sched_getcpu();
  int cpu_dest=1;
  if(nr != cpu_src)
     cpu_dest = nr;
 cpu_set_t cpu_mask;
 CPU_ZERO(&cpu_mask);
 CPU_SET(cpu_dest, &cpu_mask);
printf("nr cpu %d\n",cpu_dest);

__ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);


 // struct timespec ts = { .tv_sec = 0, .tv_nsec = 11000 };

  for (cnt = 0; cnt < ROUNDS; ++cnt)
    {
      if (pthread_mutex_lock (&lock) != 0)
	{
	  printf ("thread %d: failed to get the lock\n", nr);
	  return (void *) 1l;
	}

	for(i=0;i<999;i++){
	//do something
		kounter++;
	}

      if (pthread_mutex_unlock (&lock) != 0)
	{
	  printf ("thread %d: failed to release the lock\n", nr);
	  return (void *) 1l;
	}

      //nanosleep (&ts, NULL);
     //forsleep();
     for(i=0;i<99;i++){
     }
    }

  return NULL;
}


int
mutex7 (int thr, int rd)
{
  uint64_t start = 0;
uint64_t end = 0;
start = rdtsc();
  if(thr > 0 ) N = thr;

  if(rd > 0) ROUNDS = rd;
  pthread_mutexattr_t a;

  if (pthread_mutexattr_init (&a) != 0)
    {
      puts ("mutexattr_init failed");
      exit (1);
    }

  if (pthread_mutexattr_settype (&a, TYPE) != 0)
    {
      puts ("mutexattr_settype failed");
      exit (1);
    }

#ifdef ENABLE_PI
  if (pthread_mutexattr_setprotocol (&a, PTHREAD_PRIO_INHERIT) != 0)
    {
      puts ("pthread_mutexattr_setprotocol failed");
      return 1;
    }
#endif

  int e = pthread_mutex_init (&lock, &a);
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

  if (pthread_mutexattr_destroy (&a) != 0)
    {
      puts ("mutexattr_destroy failed");
      return 1;
    }

  pthread_attr_t at;
  pthread_t th[N];
  int cnt;

  if (pthread_attr_init (&at) != 0)
    {
      puts ("attr_init failed");
      return 1;
    }

  if (pthread_attr_setstacksize (&at, 1 * 1024 * 1024) != 0)
    {
      puts ("attr_setstacksize failed");
      return 1;
    }

  if (pthread_mutex_lock (&lock) != 0)
    {
      puts ("locking in parent failed");
      return 1;
    }

  for (cnt = 0; cnt < N; ++cnt)
    if (pthread_create (&th[cnt], &at, tf, (void *) (long int) cnt+1) != 0)
      {
	printf ("creating thread %d failed\n", cnt);
	return 1;
      }

  if (pthread_attr_destroy (&at) != 0)
    {
      puts ("attr_destroy failed");
      return 1;
    }

  if (pthread_mutex_unlock (&lock) != 0)
    {
      puts ("unlocking in parent failed");
      return 1;
    }
 
  for (cnt = 0; cnt < N; ++cnt){
	printf("Join %d\n",cnt);	 
   if (pthread_join (th[cnt], NULL) != 0)
      {
	printf ("joining thread %d failed\n", cnt);
	return 1;
      }
  }
  printf("kounter %d\n",kounter);
  end = rdtsc();
//printf("compute time {%ld} \n",(end-start));
  return 0;
}

#define TIMEOUT 60
