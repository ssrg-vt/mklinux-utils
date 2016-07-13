/* POPCORN Threadpool Implementation File
 * Author: bielsk1@vt.edu
 * Date: 07/11/16
 * Version: 0.1
 */
#include "popcorn_threadpool.h"
#include "omp.h"



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
