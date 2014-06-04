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
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include<sched.h>
#include<syscall.h>
#include "tst-barrier.h"
//#include "switch_cpu.h"


static inline int switch_cpu1(int cpuid)
{
	 printf ("child %d: lock cpu{%lu} \n",syscall(SYS_gettid),0);

	int __ret;
	int cpu_src = sched_getcpu();
	int cpu_dest=1;

	if(cpuid != cpu_src)
		     cpu_dest = cpuid;

	cpu_set_t cpu_mask;
	CPU_ZERO(&cpu_mask);
	CPU_SET(cpu_dest, &cpu_mask);


	 __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);


	  return __ret;

}

int
barrier1 (int cpu)
{
  pthread_barrier_t b;
  int e;
  int cnt;

   printf ("&bar = %p  \n", &b);

  e = pthread_barrier_init (&b, NULL, 0);
  if (e == 0)
    {
      puts ("barrier_init with count 0 succeeded");
      return 1;
    }
  if (e != EINVAL)
    {
      puts ("barrier_init with count 0 didn't return EINVAL");
      return 1;
    }

  if (pthread_barrier_init (&b, NULL, 1) != 0)
    {
      puts ("real barrier_init failed");
      return 1;
    }

  switch_cpu1(cpu);

  for (cnt = 0; cnt < 10; ++cnt)
    {
      e = pthread_barrier_wait (&b);

      if (e != PTHREAD_BARRIER_SERIAL_THREAD)
	{
	  puts ("barrier_wait didn't return PTHREAD_BARRIER_SERIAL_THREAD");
	  return 1;
	}
    }

  if (pthread_barrier_destroy (&b) != 0)
    {
      puts ("barrier_destroy failed");
      return 1;
    }

  return 0;
}

