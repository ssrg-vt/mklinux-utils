#ifndef POPCORN_THREADPOOL_H
#define POPCORN_THREADPOOL_H

#define MAX_THREADS 24
#define MAX_QUEUE 65536

#ifndef NUMA
# ifndef _GNU_SOURCE
# define _GNU_SOURCE
# endif
#include <sched.h>
#endif

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>

#ifdef NUMA
#include <numa.h>
#endif

#include <assert.h>

typedef struct threadpool_t threadpool_t;

/* A Threadpool "Queue-Task" struct */
typedef struct {
	void (*fn)(void *);
	void *args;
} threadpool_task_t;

/* The Threadpool struct*/
struct threadpool_t {
	pthread_mutex_t lock; // The lock to keep queue threadsafe
	pthread_cond_t notify; // Lock notification variable???
	pthread_t *threads; //??
	threadpool_task_t *queue; // The task queue
	int thread_count; // Num threads currently up for grabs
	int queue_size; //Capacity of Queue

	//Queue current indexes
	int head; // Current position of head
	int tail; // Current position of tail
	int count; // Current number of tasks in queue waiting to be completed
	//Queue status
	int shutdown; // Queue has marked queue to be shutdown
	int started; // Number of worker threads started
};

typedef enum{
	threadpool_invalid = -1,
	threadpool_lock_failure = -2,
	threadpool_queue_full = -3,
	threadpool_thread_failure = -4,
	threadpool_shutdown = -5
} threadpool_error_t;

//Destroy flags?
typedef enum {
	graceful_shutdown = 1
} threadpool_shutdown_t;


//THREADPOOL FUNCTIONS
/* Create the Threadpool */
threadpool_t *threadpool_create( int num_threads, int queue_size);
/* Add a new task to be completed by the Threadpool */
int threadpool_add( threadpool_t *pool, void (*fn)(void *), void *args);
/* Done working with Threadpool, Destroy all relevant parts of it */
int threadpool_destroy( threadpool_t *pool);
/* Deallocate Memory of the actual pool (called by theadpool_destroy) */
int threadpool_free(threadpool_t * pool);
/* Start Master Threadpool thread to wait for new tasks to arrive and assign tasks to pool */
void *threadpool_run(void *threadpool);
/* Get current # of threads spawned within pool */
int threadpool_getNumThreads(threadpool_t *pool);

/*ADDED LINUX BACKEND*/
typedef int (*bomp_thread_func_t)(void *);

void backend_set_numa(unsigned id);
void backend_run_func_on(int core_id, void* cfunc, void *arg);
void* backend_get_tls(void);
void backend_set_tls(void *data);
void* backend_get_thread(void);
void backend_init(void);
void backend_exit();
void backend_thread_exit(void);
struct thread *backend_thread_create_varstack(bomp_thread_func_t start_func,
                                              void *arg, size_t stacksize);
/*END LINUX BACKEND */

#endif /*POPCORN_THREADPOOL_H*/
