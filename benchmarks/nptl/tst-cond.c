#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include "tst-cond.h"


int main(int args,char *argv[])
{

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
		cond2(thread);
		if(s!=0)
		   break;
	case 3:
		printf("exec cond 3 thread{%d}\n",thread);
		cond3(thread);
		if(s!=0)
		   break;
	case 7:
		printf("exec cond 7 thread{%d}\n",thread);
		cond7(thread);
		if(s!=0) break;
	case 10:
		printf("exec cond 10 thread{%d} round{%d}\n",thread,round);
		cond10(thread,round);
		if(s!=0) break;
	case 14:
		printf("exec cond 14 cpu{%d}\n",cpu);
		cond14(cpu);
		if(s!=0) break;
	case 16:
		printf("exec cond16 thread{%d}\n",thread);
		cond16(thread);
		if(s!=0) break;	
	case 18:
		printf("exec cond18 thread{%d}\n",thread);
		cond18(thread);
		if(s!=0) break;
	case 22:
		printf("exec cond22 cpu{%d}\n",cpu);
		cond22(cpu);
		if(s!=0) break;

	
	default:
		printf("Enter 2,3,7,10,14,16,18,22\n");
}
}
