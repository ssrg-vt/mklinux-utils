#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include "tst-mutex.h"


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
	case 1:	
		printf("executing mutex1\n");
		mutex1();
		if(s!=0)
		   break;
	case 2:
		printf("exec mutex 2 cpu {%d} \n",cpu);
		mutex2(cpu);
		if(s!=0)
		   break;
	case 3:
		printf("exec mutex 3 cpu {%d} \n",cpu);
		mutex3(cpu);
		if(s!=0) break;
	case 7:
		printf("exec mutex 7 thread {%d} round {%d} \n",thread,round);
		mutex7(thread,round);
		if(s!=0) break;
	case 8:
		printf("exec mutex 8 cpu {%d} \n",cpu);
		mutex8(cpu);
		if(s!=0) break;
	case 6:
		printf("exec mutex6\n");
		mutex6();
		if(s!=0) break;

	default:
		printf("Enter 1,2,3,6,7,8\n");
}
}
