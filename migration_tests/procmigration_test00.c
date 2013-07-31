#define _GNU_SOURCE
#include <sched.h>  
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#define POPCORN

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}


int main()
{
  int __ret;
  pid_t pid;
  int cpu_src = sched_getcpu();
  char buf[512];
  if (cpu_src == -1)
    perror("sched_getcpu");
  
  int total_cpus = sysconf(_SC_NPROCESSORS_CONF) - 1;
#ifdef POPCORN
  int cpu_dest = (cpu_src +1);
#else
  int cpu_dest = (cpu_src +1) % total_cpus;
#endif
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu_dest, &cpu_mask);
  
  uint64_t start = rdtsc();
  uint64_t end = 0;
  pid = fork();
  if ( !pid ) {
    __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
    //while(cpu_dest != sched_getcpu()) {};
    end = rdtsc(); 
    sprintf(buf,"echo child: migration time{%ld} start{%ld} end{%ld} pid{%d} > /dev/kmsg",
            (end - start), start, end, getpid());
    system(buf);
    return 0;
  }
  else
    printf("pids child: %d parent: %d\n",
      pid, getpid());

  return 0;
}

