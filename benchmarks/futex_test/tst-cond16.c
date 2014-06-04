/* Copyright (C) 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2004.

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<syscall.h>

#include "tst-cond.h"

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
bool n, exiting;
FILE *f;
int count;

void *
tf (void *dummy)
{
  bool loop = true;
  int cpu = (int)dummy;
   printf ("child %lu: lock cpu{%lu} \n",syscall(SYS_gettid),cpu+1);

   int __ret;
   int cpu_src = sched_getcpu();
   int cpu_dest= cpu+1;
   cpu_set_t cpu_mask;
   CPU_ZERO(&cpu_mask);
  CPU_SET(cpu_dest, &cpu_mask);
  if(cpu_src != cpu)
   __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);


  while (loop)
    {
      pthread_mutex_lock (&lock);
      while (n && !exiting)
	pthread_cond_wait (&cv, &lock);
      n = true;
      pthread_mutex_unlock (&lock);

      //fputs (".", f);

      pthread_mutex_lock (&lock);
      n = false;
      if (exiting)
	loop = false;
#ifdef UNLOCK_AFTER_BROADCAST
      pthread_cond_broadcast (&cv);
      pthread_mutex_unlock (&lock);
#else
      pthread_mutex_unlock (&lock);
      pthread_cond_broadcast (&cv);
#endif
    }

  return NULL;
}

int cond16 (int t)
{

  f = fopen ("/dev/null", "w");
  if (f == NULL)
    {
      printf ("couldn't open /dev/null, %m\n");
      return 1;
    }

  count = 2;
  
  if(t>0)
    count = t;

  pthread_t th[count];
  int i, ret;
  for (i = 0; i < count; ++i)
    if ((ret = pthread_create (&th[i], NULL, tf, (void*)i)) != 0)
      {
	errno = ret;
	printf ("pthread_create %d failed: %m\n", i);
	return 1;
      }

  struct timespec ts = { .tv_sec = 10, .tv_nsec = 0 };
  while (nanosleep (&ts, &ts) != 0);

  pthread_mutex_lock (&lock);
  exiting = true;
  pthread_mutex_unlock (&lock);

  for (i = 0; i < count; ++i)
    pthread_join (th[i], NULL);

  fclose (f);
  return 0;
}

#define TIMEOUT 40
