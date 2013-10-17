#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>


#define L3_SIZE 6291456 //6144 kB
#define CACHE_LINE 64 //64 B
#define PAD_SIZE CACHE_LINE-4 //CACHE_LINE-sizeof(int)
//#define ARRAY_SIZE 2*(L3_SIZE/CACHE_LINE)
#define ARRAY_SIZE 100

typedef struct padded_int_value{

	int value;
	char pad[PAD_SIZE];

}pad_int;

int _dst;
int iterations;
int coordination;
unsigned long *times1;
unsigned long *times2;
pad_int * array1;
pad_int * array2;
int wait;


int cfn1( void*blah ) {
   
    cpu_set_t cpu_mask;
    int i,j,app;
    pad_int* scorre;
    long* scorre2;
    struct timeval start;
    struct timeval end;


    printf("Migrating thread1...\n");

    CPU_ZERO(&cpu_mask);
    CPU_SET(_dst,&cpu_mask);

    if(sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask)== -1){
	printf("No set affinity thread1...\n");
    }


    __sync_add_and_fetch(&wait,1);

    while(wait<=1){}

    scorre2= times1;
    for(i=0;i<iterations;i++){
        scorre=array1;
	
	gettimeofday(&start,NULL);
	for(j=0;j<ARRAY_SIZE;j++){
		app=scorre->value;
		scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));
	}
	
	gettimeofday(&end,NULL);
	*scorre2= (1000000 *end.tv_sec+ end.tv_usec)-(1000000 *start.tv_sec+ start.tv_usec);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
    }

    __sync_add_and_fetch(&coordination,1);

    while(1);
}

int cfn2( void*blah ) {
   
    cpu_set_t cpu_mask;
    int i,j,app;
    pad_int* scorre;
    long* scorre2;
    struct timeval start;
    struct timeval end;

	
    printf("Migrating thread2...\n");

    CPU_ZERO(&cpu_mask);
    CPU_SET(_dst,&cpu_mask);

    if(sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask)== -1){
	printf("No set affinity thread2...\n");
	
    }
    
    __sync_add_and_fetch(&wait,1);

    while(wait<=1){}
    
    scorre2= times2;
    for(i=0;i<iterations;i++){
        scorre=array2;
	
	gettimeofday(&start,NULL);
	for(j=0;j<ARRAY_SIZE;j++){
		app=scorre->value;
		scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));
	}
	
	gettimeofday(&end,NULL);
	*scorre2= (1000000 *end.tv_sec+ end.tv_usec)-(1000000 *start.tv_sec+ start.tv_usec);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
    }

    __sync_add_and_fetch(&coordination,1);
  
    while(1);
}


int main( int argc, char**argv ) {
    int i;
    long tot;
    pad_int* scorre;
    long* scorre2;
    const int STACK_SZ = 65536;

    if(argc == 3) {
        _dst = atoi(argv[1]);
	iterations = atoi(argv[2]);
    }
    else{
 	printf("arg: dest_cpu number_iterations\n");
        exit(0);
    }

    if( _dst == sched_getcpu()) {
        printf("dst == current, exiting\n");
        exit(0);
    }

    if( iterations <=0 ) {
        printf("iterations <=0 , exiting\n");
        exit(0);
    }

    array1= (pad_int*)malloc(ARRAY_SIZE*sizeof(pad_int));
   
    if(!array1){
 	printf("Unable to allocate array\r\n");
        return 0;
    }

    array2= (pad_int*)malloc(ARRAY_SIZE*sizeof(pad_int));
   
    if(!array2){
 	printf("Unable to allocate array\r\n");
        return 0;
    }

    
    times1= (long*)malloc(iterations*sizeof(long));
   
    if(!times1){
 	printf("Unable to allocate time array\r\n");
        return 0;
    }

    times2= (long*)malloc(iterations*sizeof(long));
   
    if(!times2){
 	printf("Unable to allocate time array\r\n");
        return 0;
    }

    scorre=array1;
    for(i=0;i<ARRAY_SIZE;i++){
	scorre->value=0;
	scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));

    }

    scorre=array2;
    for(i=0;i<ARRAY_SIZE;i++){
	scorre->value=0;
	scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));

    }

    void* stack1 = malloc(STACK_SZ);
    if(!stack1) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }

    void* stack2 = malloc(STACK_SZ);
    if(!stack2) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }
    
    coordination=0;
	
    wait=0;

    printf("Cloning first thread...\n");
 
    clone(cfn1, stack1+STACK_SZ, CLONE_THREAD|CLONE_SIGHAND|CLONE_VM, NULL);

    printf("Cloning second thread...\n");
 
    clone(cfn2, stack2+STACK_SZ, CLONE_THREAD|CLONE_SIGHAND|CLONE_VM, NULL);
	
    while(coordination!=2){}
	
    printf("Printing...\n");

    tot=0;
    scorre2=times1;
    for(i=0;i<iterations;i++){
	printf("iteration %i: %ld\n",i,(*scorre2));
	tot=tot+(*scorre2);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
    }
	
    printf("total1: %ld\n",tot);
	
    tot=0;
    scorre2=times2;
    for(i=0;i<iterations;i++){
	printf("iteration %i: %ld\n",i,(*scorre2));
	tot=tot+(*scorre2);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
    }
	
    printf("total2: %ld\n",tot);
       
   while(1);
   printf("finito");

}
