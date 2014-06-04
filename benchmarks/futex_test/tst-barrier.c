#define _GNU_SOURCE
#include<stdio.h>
#include "tst-barrier.h"

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}


int main(int args,char *argv[])
{

int s=0,thread=4,round=4,cpu=0;

if(argv[1] != NULL)
	   s= ( (atoi(argv[1]) <= 0) ? 0 : atoi(argv[1]) );

if(argv[2] !=NULL)
	   thread = atoi(argv[2]);

if(argv[3] != NULL)
	   round = atoi(argv[3]);

if(argv[4] != NULL)
	   cpu = atoi(argv[4]);
uint64_t start = 0;
uint64_t end = 0;
switch(s){
	case 0:
	case 1:	
		printf("exec barrier 1 cpu{%d} \n",cpu);
		start = rdtsc();
		barrier1(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0)
		   break;
	case 2:
		printf("exec barrier 2 thread{%d} round{%d} \n",thread, round);
		start = rdtsc();
		barrier3(thread,round);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 4:
		printf("exec barrier 4 thread{%d} \n",thread);
		start = rdtsc();
		barrier4(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	default:
		printf("Enter 1,2,4\n");

}
}
