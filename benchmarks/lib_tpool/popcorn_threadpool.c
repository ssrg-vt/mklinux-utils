/* POPCORN Threadpool Implementation File
 * Author: bielsk1@vt.edu
 * Date: 07/11/16
 * Version: 0.1
 */
#include "popcorn_threadpool.h"
#include "omp.h"

//GLOBAL POOL VAR
threadpool_t *pool = NULL;


/* Worker? */
//static void *threadpool_thread(void *threadpool);


/*
 * num_threads: Number of threads that should be in the the queue
 * queue_size: Max size of queue?
 */
threadpool_t *threadpool_create( int num_threads, int queue_size){
	//Check if app killed all queue threads?
	if(num_threads <= 0 || queue_size <= 0 ){
		return NULL;
	}

	threadpool_t *pool;
	int i;
	
	if((pool = (threadpool_t *) malloc(sizeof(threadpool_t))) == NULL){
	  if(pool){
	    threadpool_free(pool);
	  }
	  return NULL;
	}

	/*Initialize threadpool struct components*/
	pool->thread_count = 0;
	pool->queue_size = queue_size;
	pool->head = pool-> tail = pool->count = 0;
	pool->shutdown = pool->started = 0;
	
	/*Allocate the master thread and its task queue*/
	pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
	pool->queue = (threadpool_task_t *) malloc(sizeof(threadpool_task_t) * queue_size);
	
	/*Initialize Queue lock & notification variable*/
	if( (pthread_mutex_init(&(pool->lock), NULL) != 0) ||
		(pthread_cond_init(&(pool->notify), NULL) != 0) ||
		(pool->threads == NULL) ||
		(pool->queue == NULL)
	){
		if(pool){
			threadpool_free(pool);
		}
		return NULL;
	}//end if

	/*Start the worker threads*/
	for(i = 0; i < num_threads; i++){
	  if(pthread_create(&(pool->threads[i]), NULL, threadpool_run, (void*)pool) != 0){
	    //failed somewhere destroy and exit
	    threadpool_destroy(pool);
	    return NULL;
	  }//end if
	  //success and increment counters
	  pool->thread_count++;
	  pool->started++;
	}

	//return the newly created threadpool
	return pool;
}//END threadpool_create

/* Add a task to completed by the threadpool*/
int threadpool_add( threadpool_t *pool, void (*fn)(void *), void *args){
	int result = 0;
	int next;
	
	if(pool == NULL || fn == NULL){
		return threadpool_invalid;
	}

	if(pthread_mutex_lock(&(pool->lock)) != 0){
		return threadpool_lock_failure;
	}

	next = (pool->tail+1) % pool->queue_size;

	do {
	  if(pool->count == pool->queue_size){
		result = threadpool_queue_full;
		break;
	  }

	  if(pool->shutdown){
		result = threadpool_shutdown;
		break;
	  }
	  
	  /*Add the given task to the queue*/
	  pool->queue[pool->tail].fn = fn;
	  pool->queue[pool->tail].args = args;
	  pool->tail = next;
	  pool->count = pool->count + 1;

	  /*pthread_cond_broadcast*/
	  if(pthread_cond_signal(&(pool->notify)) != 0){
	    result = threadpool_lock_failure;
	    break;
	  }
	} while(0);
	
	if(pthread_mutex_unlock(&(pool->lock)) != 0){
	  result = threadpool_lock_failure;
	}
	
	printf("%s: result %d\n",__func__,result);
	return result;
}//END threadpool_add

/* Destroy the threadpool */
int threadpool_destroy( threadpool_t *pool){
	int i, result= 0;
	if( pool == NULL){
	  return threadpool_invalid;
	}

	if(pthread_mutex_lock(&(pool->lock)) != 0){
	  return threadpool_lock_failure;
	}

	do{
	  /*Someone signaled Shutdown already*/
	  if(pool->shutdown){
	    result = threadpool_shutdown;
	    break;
	  }

	  pool->shutdown = 1;

	  /*Wake up all work threads*/
	  if((pthread_cond_broadcast(&(pool->notify)) != 0) || 
		(pthread_mutex_unlock(&(pool->lock)) != 0))  {
	    result = threadpool_lock_failure;
	  }
	
	  /*Join All Worker Threads */
	  for(i = 0 ; i < pool->thread_count ; i++){
	    if(pthread_join(pool->threads[i], NULL) != 0){
		result = threadpool_thread_failure;
	    }
	  }
	} while(0);

	/* Deallocate Threadpool */
	if(!result){
	  threadpool_free(pool);
	}
	return result;
}//END threadpool_destroy

int threadpool_free(threadpool_t *pool){
	if(pool == NULL || pool->started > 0){
	  return -1;
	}
	/* Did threadpool_create succeed? */
	if(pool->threads){
	  free(pool->threads);
	  free(pool->queue);
	
	  pthread_mutex_lock(&(pool->lock));
	  pthread_mutex_destroy(&(pool->lock));
	  pthread_cond_destroy(&(pool->notify));
	}
	free(pool);
	return 0;
}//END threadpool_free

void* threadpool_run(void* threadpool){
	threadpool_t *pool = (threadpool_t*) threadpool;
	threadpool_task_t task;

	while(1){
	  /* Attempt to Lock */
	  pthread_mutex_lock(&(pool->lock));
	
	  while((pool->count == 0) && (!pool->shutdown)){
	    pthread_cond_wait(&(pool->notify), &(pool->lock));
	  }

	  if((pool->shutdown == graceful_shutdown) && (pool->count == 0)){
	    break;
	  }

	  /* Grab a task from FRONT of queue*/
	  task.fn = pool->queue[pool->head].fn;
	  task.args = pool->queue[pool->head].args;
	  pool->head = (pool->head +1) % pool->queue_size;
	  pool->count = pool->count - 1;

	  /* Unlock */
	  pthread_mutex_unlock(&(pool->lock));

	  /* Start Task */
	  (*(task.fn))(task.args);
	}//END Inifinite loop

	pool->started--;
	pthread_mutex_unlock(&(pool->lock));
	pthread_exit(NULL);

}//END threadpool_run

int threadpool_getNumThreads(threadpool_t *pool){
  return pool->thread_count;
}//END threadpool_getNumThreads


/**
 * LINUX BACKEND
 * \brief Implementation of backend functions on Linux
 */

void backend_set_numa(unsigned id)
{
#if 0
#ifdef NUMA
    struct bitmask *bm = numa_allocate_cpumask();
    numa_bitmask_setbit(bm, id);
    numa_sched_setaffinity(0, bm);
    numa_free_cpumask(bm);
#else
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
#endif
#endif
}

void backend_run_func_on(int core_id, void* cfunc, void *arg)
{
printf("%s: Adding task 0x%d\n",__func__,&cfunc);

//Add function to threadpool task queue
threadpool_add(pool, cfunc, arg);
}

static pthread_key_t pthread_key;

void *backend_get_tls(void)
{
    return pthread_getspecific(pthread_key);
}
 
void backend_set_tls(void *data) {
    assert(data);
    pthread_setspecific(pthread_key, data);
}

void *backend_get_thread(void) {
    return (void *)pthread_self();
}

static int remote_init(void *dumm) {
    return 0;
}

void backend_span_domain_default(int nos_threads) {
    /* nop */
}

void backend_span_domain(int nos_threads, size_t stack_size) {
    /* nop */
}

void backend_init(void) {
/*    int r = pthread_key_create(&pthread_key, NULL);
    if (r != 0) {
        printf("pthread_key_create fa2iled\n");
        //printf("%s:r PO PO 0\n",__func__);
    }*/
char s[512];
FILE *fd;
int num_cores=4;

/*fd = fopen("/proc/cpuinfo", "r");
if (fd == NULL) {
  num_cores = 4;
} else {
  while (fgets(s, 512, fd) != NULL) {
    if (strstr(s, "GenuineIntel") != NULL || strstr(s, "AuthenticAMD") != NULL){
	num_cores++;
    }
  fclose(fd);
}*/
//assert((pool = threadpool_create(1,MAX_QUEUE)) == 0);
pool = threadpool_create(num_cores,MAX_QUEUE);
printf("%s: Threadpool Initiated, %d cores detected\n",__func__,num_cores);
}

void backend_exit(void) {
	assert(threadpool_destroy(pool) == 0);

#ifdef SHOW_PROFILING   
        dump_sched_self();
#endif
}

void backend_create_time(int cores) {
    /* nop */
}

void backend_thread_exit(void) {
}

struct thread *backend_thread_create_varstack(bomp_thread_func_t start_func,
                                              void *arg, size_t stacksize) {
    start_func(arg);
    return NULL;
}

