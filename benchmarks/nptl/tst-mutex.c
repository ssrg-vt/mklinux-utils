#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include "tst-mutex.h"


static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}



int main(int args,char *argv[])
{

int s=0,thread=4,round=4,cpu=-1;
uint64_t start = 0;
uint64_t end = 0;
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
	case 1:	
		printf("executing mutex1\n");
		start = rdtsc();
		mutex1();
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;
	case 2:
		printf("exec mutex 2 cpu {%d} \n",cpu);
		start = rdtsc();
		mutex2(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;
	case 3:
		printf("exec mutex 3 cpu {%d} \n",cpu);
		start = rdtsc();
		mutex3(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;
	case 7:
		printf("exec mutex 7 thread {%d} round {%d} \n",thread,round);
		start = rdtsc();
		mutex7(thread,round);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;
	case 8:
		printf("exec mutex 8 cpu {%d} \n",cpu);
		start = rdtsc();
		mutex8(cpu);
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;
	case 6:
		printf("exec mutex6\n");
		start = rdtsc();
		mutex6();
		end = rdtsc();
		printf("compute time {%ld} \n",(end-start));if(s!=0) break;

	default:
		printf("Enter 1,2,3,6,7,8\n");
}
}
