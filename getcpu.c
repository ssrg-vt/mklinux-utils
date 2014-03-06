// Copyright (C) Antonio Barbalace, SSRG VT, 2014

#include <stdio.h>

#define LIBC_SUPPORT
#ifndef LIBC_SUPPORT
# include <linux/getcpu.h>
#else
# define _GNU_SOURCE
# include <utmpx.h>
#endif


static inline int _getcpu(int* cpuid)
{
#ifndef LIBC_SUPPORT
  unsigned int cpu=(unsigned int)-1;
  return  getcpu(&cpu, 0, 0);
#else
  if (cpuid) {
    *cpuid = sched_getcpu();
    return 0;
  }
  return -1;
#endif
}


int main(int argc, char* argv[])
{
  unsigned int cpu=(unsigned int)-1, curr_cpu;
  int i, verbose=0, ret, iter=0;

  if (argc > 1)
    for (i =1; i< argc; i++)
      if (!strcasecmp(argv[i], "-h") || !strcasecmp(argv[i], "--help")) {
	printf("usage: getcpu [-h] [-v]\n"
               "Shows the id of the cpu the utility program run on.\n"
               "  -h, --help     shows this help\n"
               "  -v, --verbose  verbose output\n");
        return 0;
      }
      else if (!strcasecmp(argv[i], "-v") || !strcasecmp(argv[i], "--verbose")) {
        verbose =1;
      }

  ret = _getcpu(&cpu);
  if (cpu == (unsigned int)-1) {
    printf("error %d cpu %u getcpu is possibly not supported on the system.\n", ret, cpu);
    goto _exit;
  }

  _getcpu(&curr_cpu);
  while ( curr_cpu != cpu ) {
    cpu = curr_cpu;
    _getcpu(&curr_cpu);
    iter++;
  }

  if (verbose)
    printf("cpu: %d iterations: %d\n", cpu, iter);
  else
    printf("%d\n", cpu);

_exit:
  return 0;
}
