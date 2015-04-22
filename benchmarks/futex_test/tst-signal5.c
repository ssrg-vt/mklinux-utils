/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "tst-signal.h"

static sigset_t ss;
static int parent_signal[32]={0};
static int child_signal[32]={0};

static void *
tf (void *arg)
{
int i = (long int) arg;

  sigset_t ss2;


  if (pthread_sigmask (SIG_SETMASK, NULL, &ss2) != 0)
    {
      puts ("child: sigmask failed");
      exit (1);
    }
int __ret;
int cpu_src = sched_getcpu();
int cpu_dest= i;

cpu_set_t cpu_mask;
CPU_ZERO(&cpu_mask);
CPU_SET(cpu_dest, &cpu_mask);


 __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);


  for (i = 1; i < 32; ++i)
    if (sigismember (&ss, i) && ! sigismember (&ss2, i))
      {
	parent_signal[i]++;
	exit (1);
      }
    else if (! sigismember (&ss, i) && sigismember (&ss2, i))
      {
	child_signal[i]++;
	exit (1);
      }

  return NULL;
}


int
sig5(int cpu)
{
  int i=0;
  sigemptyset (&ss);
  sigaddset (&ss, SIGUSR1);
  if (pthread_sigmask (SIG_SETMASK, &ss, NULL) != 0)
    {
      puts ("1st sigmask failed");
      exit (1);
    }

  pthread_t th;
  if (pthread_create (&th, NULL, tf, cpu) != 0)
    {
      puts ("1st create failed");
      exit (1);
    }

  void *r;
  if (pthread_join (th, &r) != 0)
    {
      puts ("1st join failed");
      exit (1);
    }
  printf("Before setting signals\n");
  for(i=0;i<32;i++){
	printf("signal %d Parent %d child %d \n",i,parent_signal[i],child_signal[i]);
	parent_signal[i]=0;
	child_signal[i]=0;
  }


  sigemptyset (&ss);
  sigaddset (&ss, SIGUSR2);
  sigaddset (&ss, SIGFPE);
  if (pthread_sigmask (SIG_SETMASK, &ss, NULL) != 0)
    {
      puts ("2nd sigmask failed");
      exit (1);
    }

  if (pthread_create (&th, NULL, tf, cpu) != 0)
    {
      puts ("2nd create failed");
      exit (1);
    }

  if (pthread_join (th, &r) != 0)
    {
      puts ("2nd join failed");
      exit (1);
    }

  for(i=0;i<32;i++){
	printf("signal %d Parent %d child %d \n",i,parent_signal[i],child_signal[i]);
  }

  return 0;
}
