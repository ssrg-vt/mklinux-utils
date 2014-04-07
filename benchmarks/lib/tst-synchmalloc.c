
// Copyright 2014, Marina Sadini
// partially rewritten by Antonio Barbalace

#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef PTHREAD
# include "cthread.h"
#else /* PTHREAD */
# include "pthread.h"
#endif /* !PTHREAD */

struct clone_param{
	int id;
	char** to_write;
};

const int ITERATION= 10;
int* dst;
int memory_for_thread;
volatile int wait_all_clone;
volatile int wait_all_write;
volatile int wait_for_free;

char from_int_to_mod_char(int i)
{
    int mod = (i%10);
    return ('0' + mod);
}

void* function( void* param) {
	struct clone_param* my_param= (struct clone_param*)param;
	cpu_set_t cpu_mask;
	int id= my_param->id;
	int i,j;
	char** appa;
	char* app;
	
    printf("Migrating thread %i ...\n",id);
    
    CPU_ZERO(&cpu_mask);
    CPU_SET(dst[id],&cpu_mask);

#ifndef PTHREAD
    cthread_setaffinity_np(0,sizeof(cpu_set_t),&cpu_mask);
#else /* PTHREAD */  
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);
#endif /* !PTHREAD */
    
    printf("after migration thread %i\n",id);
    
    __sync_add_and_fetch(&wait_all_clone,1);
    
    while(wait_all_clone!=-1){}
    
    printf("allocating to write thread %i\n",id);
   
    my_param->to_write= (char**)malloc(ITERATION*sizeof(char*));
	if(my_param->to_write==NULL){
		printf("error malloc array to write thread: %i\n",id);
		return ;
	}
	
	printf("malloc array to write thread: %i\n",id);
    
    appa= my_param->to_write;
    for(i=0;i<ITERATION;i++){
		
		appa[i]= (char*) malloc((int)(memory_for_thread/ITERATION)*sizeof(char));
		if(appa[i]==NULL){
			printf("error malloc inner array %i thread: %i\n",i,id);
			for(j=i-1;j>=0;j--)
				free(appa[j]);
			free(my_param->to_write);
			return ;
		}
		printf("malloc inner array %i thread: %i\n",i,id);
	}

	for(i=0;i<ITERATION;i++){
		app= my_param->to_write[i];
		for(j=0;j<(int)(memory_for_thread/ITERATION);j++)
			app[j]= from_int_to_mod_char(id);
	}
	
	
	__sync_add_and_fetch(&wait_all_write,1);
	
	while(wait_all_write!=-1){}
	
	for(i=0;i<ITERATION;i++){
		
		free(my_param->to_write[i]);
	
	}
	
	free(my_param->to_write);
	
	__sync_add_and_fetch(&wait_for_free,1);

#ifdef PTHREAD
	pthred_exit((void*) 0);
#endif /* PTHREAD */	
}

int main (int argc, char** argv) {
	int num_thread,i,j,k;
	//const int STACK_SIZE= 65536;
	char* stack,*app_stack,**app_write,*app;
	struct clone_param* param;
	int memory,res;
#ifndef PTHREAD	
	cthread_t* threads,*app_threads;
#else /* PTHREAD */	
	pthread_t* pthreads,*papp_threads;
#endif /* !PTHREAD */	
	
	printf("Test dinamic virtual memory management\n");

	if(argc<=2){
		printf("Usage: amount of memory to allocate, number thread, for each thread: dst\n\n");
		exit(0);
	}
	
	memory= atoi(argv[1]);
	if(memory<=0) {
		printf("Usage: amount of memory to allocate, number thread, for each thread: dst\n");
		exit(0);
	}
	
	num_thread= atoi(argv[2]);
	if(num_thread<=0) {
		printf("Usage: amount of memory to allocate, number thread, for each thread: dst\n");
		exit(0);
	}
	
	memory_for_thread= memory/num_thread;
	printf("memory for each thread: %i\n",memory_for_thread);
	printf("size inner array: %i\n",(int)memory_for_thread/ITERATION);
	
	if(((int)memory_for_thread/ITERATION ) == 0){
		printf("Not enough memory\n");
		exit(0);
	}

#ifndef PTHREAD
	unsigned long saved_context = cthread_initialize();
#endif
	
	dst= (int*) malloc(sizeof(int)*num_thread);
	if(dst==NULL){
		printf("Impossible to malloc dst array \n");
		exit(0);
	}
	for(i=0;i<num_thread;i++){
		dst[i]= atoi(argv[i+3]);
		if(dst[i]<0){
			printf("Usage: number thread, for each thread: dst\n");
			free(dst);
			exit(0);
		}
	}
	
	/*stack= (char*) malloc(sizeof(char)*num_thread*STACK_SIZE);
	if(stack==NULL){
		printf("Impossible to malloc stack array \n");
		free(dst);
		exit(0);
	}*/
	
	param= (struct clone_param*) malloc(sizeof(struct clone_param)*num_thread);
	if(param==NULL){
		printf("Impossible to malloc param array \n");
		free(dst);
		free(stack);
		exit(0);
	}
	
	wait_all_clone= 0;
	
	//app_stack= stack;
#ifndef PTHREAD	
	threads= (cthread_t* ) malloc(sizeof(cthread_t)*num_thread);
	if(threads==NULL){
		printf("Impossible to malloc threads array \n");
		free(dst);
		exit(0);
	}
	app_threads= threads;
#else /* PTHREAD */
	pthreads= (pthread_t* ) malloc(sizeof(pthread_t)*num_thread);
	if(pthreads==NULL){
		printf("Impossible to malloc pthreads array \n");
		free(dst);
		exit(0);
	}
	papp_threads= pthreads;
#endif /* !PTHREAD */
	for(i=0;i<num_thread;i++){
		param[i].id= i;
		
		//clone(function, app_stack+STACK_SIZE, CLONE_THREAD|CLONE_SIGHAND|CLONE_VM, (void*)&param[i]);
		//app_stack= app_stack+STACK_SIZE;

#ifndef PTHREAD		
		res= cthread_create(app_threads, 0, function, (void*)&param[i]);
		app_threads= (cthread_t* ) ((char*)app_threads+ sizeof(cthread_t)); //TODO rewrite!!!
#else /* PTHREAD */		
		res= pthread_create(papp_threads, 0, function, (void*)&param[i]);
		papp_threads= (pthread_t* ) ((char*)papp_threads+ sizeof(pthread_t)); //TODO rewrite!!!
#endif /* !PTHREAD */		
	}
	
	while(wait_all_clone!=num_thread){}
	
	wait_all_write= 0;
	wait_all_clone= -1;
	
	while(wait_all_write!=num_thread){}

	printf("threads wrote:\n");
	
	for(i=0;i<num_thread;i++){
		printf("thread %i in cpu %i:\n",param[i].id,dst[i]);
		app_write= param[i].to_write;
		if(app_write==NULL){
			printf("app_write of %i is NULL \n",i);
			exit(0);
		}
		for(j=0;j<ITERATION;j++){
			app= app_write[j];
			for(k=0;k<(int)(memory_for_thread/ITERATION);k++)
				printf("%c",app[k]);
		}
		printf("\n");
	}

	wait_for_free= 0;
	wait_all_write= -1;
	
	while(wait_for_free!=num_thread){}
	
#ifndef PTHREAD	
	cthread_restore(saved_context);
#endif	
	
	free(dst);
	free(param);
	//free(stack);
#ifndef PTHREAD	
	free(threads);
#else /* PTHREAD */
	free(pthreads);
#endif /* !PTHREAD */
}
