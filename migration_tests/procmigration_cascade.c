/*
antonio barbalace 2014

process cascade migration test
*/

#define _GNU_SOURCE
#include <sched.h>  
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#define POPCORN

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}

void do_a_test(long int mem_sz, int where)
{
    char* m;
    uint64_t start = rdtsc();
    uint64_t end;
    int cpu_src = sched_getcpu();
    int cpu_dest = where;
    char buf[512] = {0};
    int i;

    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu_dest, &cpu_mask);

    m = malloc(mem_sz*sizeof(char));
    for(i = 0; i < mem_sz; i++) {
       m[i] = 0;
    }
    sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
#ifndef POPCORN	
    while(cpu_dest != sched_getcpu()) {};
#endif	
    end = rdtsc(); 
    
    sprintf(buf,"echo \"child: mem_sz{0x%lx} time{%ld} start %ld end %ld (from %d to %d)\n\"> /dev/kmsg",
	mem_sz,end-start, start, end, cpu_src, cpu_dest);
    system(buf);
    sleep(1);
}

int main(int argc, char* argv[])
{
  int i;
  long last;
  long size =1024;
 
  printf("WARN this test address one-core-per-kernel setups\n");
  
  if (argc != 2) {
    printf("must specify the last kernel id\n");
    return -1;
  }
    
  char * endptr;
  last = strtol(argv[1], &endptr, 0);
  if ((argv[1] == endptr) && (size ==0)) {
    printf("arguments error\n");
    return -1;
  }
  printf("migrating to: 0x%lx\n", last);

  for (i=0; i<last; i++) {
    sleep(1);
    do_a_test(size, i);
  }
  
  return 0;
}
