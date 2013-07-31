/*
antonio barbalace 2013

idea:

cpu_src = getcpu()
cpu_dest = cpu_src +1
read rdtsc start
clone
if child
  sched_setaffinity(cpu_dest)
  while
    cpu = getcpu()
    if cpu == cpu_dest
      break
  read rdtsc end
  print end - start
else
  join children
exit
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

void do_a_test(long int mem_sz) {
    char* m;
    uint64_t start = rdtsc();
    uint64_t end;
    int cpu_src = sched_getcpu();
    int pid;
    int total_cpus = sysconf(_SC_NPROCESSORS_CONF) - 1;
    char buf[512] = {0};
#ifdef POPCORN
    int cpu_dest = (cpu_src +1);
#else
    int cpu_dest = (cpu_src +1) % total_cpus;
#endif
    int i;
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu_dest, &cpu_mask);
    pid = fork();
    if(!pid) {
        m = malloc(mem_sz*sizeof(char));
        for(i = 0; i < mem_sz; i++) {
            m[i] = 0;
        }
        sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
        //while(cpu_dest != sched_getcpu()) {};
        end = rdtsc(); 
        sprintf(buf,"echo child: mem_sz{0x%lx} time{%ld} start %ld end %ld> /dev/kmsg",mem_sz,end-start, start, end);
        system(buf);
    } else {
        printf("parent: child pid{%d}\n", pid);
    }
}

int main(int argc, char* argv[])
{
    long int size;
 
if (argc != 2) {
printf("must specify size\n");
  return -1; }
char * endptr;
size = strtol(argv[1], &endptr, 0);
if ((argv[1] == endptr) && (size ==0)) {
printf("arguments error\n");
return -1;
}
printf("runnig with size: 0x%lx\n", size);
    do_a_test(size);
//    usleep(1000);
//    do_a_test(0x10000);
//    usleep(1000);
//    do_a_test(0x100000);
//    usleep(1000);
//    do_a_test(0x1000000);
//    usleep(1000);
//    do_a_test(0x10000000);
//    usleep(1000);
//    do_a_test(0x100000000);

  
  return 0;
}
