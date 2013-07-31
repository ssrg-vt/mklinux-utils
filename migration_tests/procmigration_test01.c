#define _GNU_SOURCE
#include <sched.h>  
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
 #include <sys/wait.h>

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    // NOTE volatile impose to the compiler to do not reordering but ooo cpus can reorder the instructions
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    // NOTE rdtscp (that also read the processor id) imposes serialization
/*    __asm__ volatile("rdtscp; "
		     "shl $32,%%rdx; "
		     "or %%rdx,%%rax"
		     : "=a"(tsc)
		     :
		     : "%rcs", "%rdx");
*/    
    return ((uint64_t)edx << 32) | eax;
}


int main()
{
  int __ret;
  pid_t pid;
  int cpu_src = sched_getcpu();
  if (cpu_src == -1)
    perror("sched_getcpu");
  
  int total_cpus = sysconf(_SC_NPROCESSORS_CONF) - 1;
  int cpu_dest = (cpu_src +1) % total_cpus;
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu_dest, &cpu_mask);
 
printf("cpu src:%d dest:%d\n", cpu_src, cpu_dest);
 
  uint64_t start = rdtsc();
  uint64_t end = 0;
  pid = fork();
  if ( !pid ) {
    __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
    while(cpu_dest != sched_getcpu()) {};
    end = rdtsc();  
    printf("child: migration time %ld (%ld:%ld) pid: %ld\n",
      (end -start), start, end, getpid());
    return 0;
  }
  else {
int __child;
wait(&__child);
    printf("pids child: %ld (%d) parent: %ld\n",
      pid, __child,getpid());
}

  return 0;
}
