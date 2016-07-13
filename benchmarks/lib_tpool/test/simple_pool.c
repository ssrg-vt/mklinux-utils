#define NUMT 4
#define QUEUE 128

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "popcorn_threadpool.h"

int tasks =0, done = 0;
pthread_mutex_t lock;

void dummy_task(void *arg){
	usleep(10000);
	pthread_mutex_lock(&lock);
	++done;
	pthread_mutex_unlock(&lock);
}

int main(int argc, char* argv[]){

	threadpool_t *pool;

	pthread_mutex_init(&lock, NULL);
	assert((pool = threadpool_create(NUMT, QUEUE)) != NULL);
	printf("Pool started with %d threads and queue size of %d\n",NUMT, QUEUE);
	
	while(threadpool_add(pool, &dummy_task, NULL) == 0){
		pthread_mutex_lock(&lock);
		tasks++;
		pthread_mutex_unlock(&lock);
	}
	
	while((tasks / 2) > done){
		usleep(10000);
	}

	assert(threadpool_destroy(pool) == 0);
	printf("Did %d tasks\n",done);
	
	return 0;
}
