#define _GNU_SOURCE
#include<stdio.h>
#include<syscall.h>
#include<sched.h>

inline int switch_cpu(int cpuid)
{
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
