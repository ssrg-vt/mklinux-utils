#define _GNU_SOURCE
#include<stdio.h>
#include "tst-barrier.h"


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

switch(s){
	case 0:
	case 1:	
		printf("exec barrier 1 cpu{%d} \n",cpu);
		barrier1(cpu);
		if(s!=0)
		   break;
	case 2:
		printf("exec barrier 2 thread{%d} round{%d} \n",thread, round);
		barrier3(thread,round);
		if(s!=0) break;
	case 4:
		printf("exec barrier 4 thread{%d} \n",thread);
		barrier4(thread);
		if(s!=0) break;
	default:
		printf("Enter 1,2,4\n");

}
}
