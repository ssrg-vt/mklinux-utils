/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include "tst-signal.h"
#include <sched.h>
#define SIGCANCEL 32
#define SIGSETXID 33
int
sig7 (int cpu)
{
  int result = 0;

  errno = 0;
  if (sigaction (SIGCANCEL, NULL, NULL) == 0)
    {
      puts ("sigaction(SIGCANCEL) did not fail");
      result = 1;
    }
  else if (errno != EINVAL)
    {
      puts ("sigaction(SIGCANCEL) did not set errno to EINVAL");
      result = 1;
    }

  errno = 0;
  if (sigaction (SIGSETXID, NULL, NULL) == 0)
    {
      puts ("sigaction(SIGSETXID) did not fail");
      result = 1;
    }
  else if (errno != EINVAL)
    {
      puts ("sigaction(SIGSETXID) did not set errno to EINVAL");
      result = 1;
    }

  return result;
}
