/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <syscall.h>

#include "tst-cond.h"


typedef int _tid;
typedef struct
  {
    pthread_cond_t cond;
    pthread_mutex_t lock;
    pthread_t h;
    _tid t;
  } T;


static volatile bool done;


static void *
tf (void *arg)
{
  puts ("child created");

  T *t =(T*) arg;

  int cpu=((T*) arg)->t ;

   if (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL) != 0
         || pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL) != 0)
      {
            puts ("cannot set cancellation options");
          return NULL; 
      }

  printf ("child %d: lock cpu{%lu} \n",syscall(SYS_gettid),(cpu+1)%4);

  switch(cpu+1);
  int __ret;
  int cpu_src = sched_getcpu();
  int cpu_dest= (cpu+1)%4;
    cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu_dest, &cpu_mask);
  
  
  __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);

  if (pthread_mutex_lock (&t->lock) != 0)
    {
      puts ("child: lock failed");
      return NULL;
    }

  done = true;

  if (pthread_cond_signal (&t->cond) != 0)
    {
      puts ("child: cond_signal failed");
      return NULL;
    }

  if (pthread_cond_wait (&t->cond, &t->lock) != 0)
    {
      puts ("child: cond_wait failed");
      return NULL;
    }

  if (pthread_mutex_unlock (&t->lock) != 0)
    {
      puts ("child: unlock failed");
      return NULL;
    }

  return NULL;
}

static int N=8;
 int
cond7 (int thr)
{
  int i;
  if(thr!=0) N=thr;

  T *t[N];
  for (i = 0; i < N; ++i)
    {
      printf ("round %d\n", i);

      t[i] = (T *) malloc (sizeof (T));
      t[i]->t = i;
      if (t[i] == NULL)
	{
	  puts ("out of memory");
	  exit (1);
	}

      if (pthread_mutex_init (&t[i]->lock, NULL) != 0
	  || pthread_cond_init (&t[i]->cond, NULL) != 0)
	{
	  puts ("an _init function failed");
	  exit (1);
	}

      if (pthread_mutex_lock (&t[i]->lock) != 0)
	{
	  puts ("initial mutex_lock failed");
	  exit (1);
	}

      done = false;

      if (pthread_create (&t[i]->h, NULL, tf, t[i]) != 0)
	{
	  puts ("pthread_create failed");
	  exit (1);
	}

      do
	if (pthread_cond_wait (&t[i]->cond, &t[i]->lock) != 0)
	  {
	    puts ("cond_wait failed");
	    exit (1);
	  }
      while (! done);

     /* Release the lock since the cancel handler will get it.  */
      if (pthread_mutex_unlock (&t[i]->lock) != 0)
	{
	  puts ("mutex_unlock failed");
	  exit (1);
	}

      if (pthread_cancel (t[i]->h) != 0)
	{
	  puts ("cancel failed");
	  exit (1);
	}

      puts ("parent: joining now");

      void *result;
      if (pthread_join (t[i]->h, &result) != 0)
	{
	  puts ("join failed");
	  exit (1);
	}
      if (result != PTHREAD_CANCELED)
	{
	  puts ("result != PTHREAD_CANCELED");
	  //	  exit (1);
	}
    }

  for (i = 0; i < N; ++i)
    free (t[i]);

  return 0;
}
