/* Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include<string.h>
#include "tst-cond.h"
#include<unistd.h>
#include <sys/types.h>
#include <stdint.h>

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static pthread_barrier_t bar;

/* Barrier data structure.  */
struct pthread_barrier
{
  unsigned int curr_event;
   int lock;
   unsigned int left;
   unsigned int init_count;
  int private;
 // char dummy[12];
};

struct pthread_barrier mystruct;

static void *
tf (void *a)
{
  int i = (long int) a;
  int err=0;
  int cpu = i;

  printf ("child %d: lock cpu{%d} pid{%d} \n", i,cpu+1,syscall(SYS_gettid));

//  switch(cpu+1);
int __ret;
int cpu_src = sched_getcpu();
int cpu_dest= i+1;

cpu_set_t cpu_mask;
CPU_ZERO(&cpu_mask);
CPU_SET(cpu_dest, &cpu_mask);


 __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);

   pthread_mutex_lock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "locking in child failed");


  int e = pthread_barrier_wait (&bar);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
	return NULL;
      //exit (1);
    }

  //printf ("child %d: wait\n", i);

  err =  pthread_cond_wait (&cond, &mut);
  if (err != 0){
   // error (EXIT_FAILURE, err, "child %d: failed to wait", i);
   return NULL; 
   }

 // printf ("child %d: woken up\n", i);

  err = pthread_mutex_unlock (&mut);
  if (err != 0){
   // error (EXIT_FAILURE, err, "child %d: unlock[2] failed", i);
  return NULL; 
  }

  return NULL;
}


//#define N 3 


int cond2 (int t)
{
  int N = t;
  pthread_t th[N];
  int i;
  int err;
  uint64_t start = 0;
  uint64_t end = 0;
  start = rdtsc();
  printf ("&cond = %p\n&mut = %p \n&bar = %p \n", &cond, &mut,&bar);
  printf ("sizeofmystruct{%d} sizeof(pthread_barrier_t{%d}\n",sizeof(mystruct),sizeof(pthread_barrier_t));
  if (pthread_barrier_init (&bar, NULL, 2) != 0)
    {
      puts ("barrier_init failed");
      exit (1);
    }


  pthread_attr_t at;

/*  if (pthread_attr_init (&at) != 0)
    {
      puts ("attr_init failed");
      return 1;
    }

  if (pthread_attr_setstacksize (&at, 1 * 1024 * 1024) != 0)
    {
      puts ("attr_setstacksize failed");
      return 1;
    }
*/
  for (i = 0; i < N; ++i)
    {
      printf ("create thread %d\n", i);

      err = pthread_create (&th[i], NULL, tf, (void *) (long int) i);
      if (err != 0)
	error (EXIT_FAILURE, err, "cannot create thread %d", i);

      printf ("wait for child %d\n", i);

      printf("1: bar (left){%u} init_count{%u}\n",*(unsigned int *)&(((struct pthread_barrier *)&bar)->left),*(unsigned int *)&(((struct pthread_barrier *)&bar)->init_count));

      /* Wait for the child to start up and get the mutex for the
	 conditional variable.  */
      int e = pthread_barrier_wait (&bar);
      if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
	{
	  puts ("barrier_wait failed");
	  exit (1);
	}
      printf("bar (left){%u} init_count{%u}\n",*(unsigned int *)&(((struct pthread_barrier *)&bar)->left),*(unsigned int *)&(((struct pthread_barrier *)&bar)->init_count));
    }

/*  if (pthread_attr_destroy (&at) != 0)
    {
      puts ("attr_destroy failed");
      return 1;
    }
*/
  puts ("get lock outselves");

  err = pthread_mutex_lock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "mut locking failed");

  puts ("broadcast");

  /* Wake up all threads.  */
  err = pthread_cond_broadcast (&cond);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: broadcast failed");

  err = pthread_mutex_unlock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "mut unlocking failed");

  /* Join all threads.  */
  for (i = 0; i < N; ++i)
    {
      printf ("join thread %d\n", i);

      err = pthread_join (th[i], NULL);
      if (err != 0)
	error (EXIT_FAILURE, err, "join of child %d failed", i);
    }

  puts ("done");
  end = rdtsc();

//  printf("compute time {%ld} \n",(end-start));
  return 0;
}


#define TEST_FUNCTION do_test ()
