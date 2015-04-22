#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include "tst-cond.h"


uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}



int main(int args,char *argv[])
{

uint64_t start = 0;
uint64_t end = 0;
int s=0,thread=4,round=4,cpu=-1;

if(argv[1] != NULL)
   s= ( (atoi(argv[1]) <= 0) ? 0 : atoi(argv[1]) );

if(argv[2] !=NULL)
   thread = atoi(argv[2]);

if(argv[3] != NULL)
   round = atoi(argv[3]);

if(argv[4] != NULL)
   cpu = atoi(argv[4]);

switch(s){
	case 0:
	case 2:	
		printf("exec cond2 thread{%d}\n",thread);
		start = rdtsc();
		cond2(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0)
		   break;
	case 3:
		printf("exec cond 3 thread{%d}\n",thread);
		start = rdtsc();
		cond3(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0)
		   break;
	case 7:
		printf("exec cond 7 thread{%d}\n",thread);
		start = rdtsc();
		cond7(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 10:
		printf("exec cond 10 thread{%d} round{%d}\n",thread,round);
		start = rdtsc();
		cond10(thread,round);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 14:
		printf("exec cond 14 cpu{%d}\n",cpu);
		start = rdtsc();
		cond14(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 16:
		printf("exec cond16 thread{%d}\n",thread);
		start = rdtsc();
		cond16(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 18:
		printf("exec cond18 thread{%d}\n",thread);
		start = rdtsc();
		cond18(thread);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;
	case 22:
		printf("exec cond22 cpu{%d}\n",cpu);
		start = rdtsc();
		cond22(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));
		if(s!=0) break;

	
	default:
		printf("Enter 2,3,7,10,14,16,18,22\n");
}
}
