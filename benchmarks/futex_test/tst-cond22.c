#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tst-cond.h"


static pthread_barrier_t b;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;


static void
cl (void *arg)
{
  pthread_mutex_unlock (&m);
}


static void *
tf (void *arg)
{ 
printf ("child %lu: lock cpu{} \n",1);

   int __ret;
   int cpu_src = sched_getcpu();
   int cpu_dest= 1;
   cpu_set_t cpu_mask;
   CPU_ZERO(&cpu_mask);
   CPU_SET(cpu_dest, &cpu_mask);


  __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);

  if (pthread_mutex_lock (&m) != 0)
    {
      //exit (1);
      return NULL;
    }
  int e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      //exit (1);
      return NULL;
    }
  pthread_cleanup_push (cl, NULL);
  /* We have to loop here because the cancellation might come after
     the cond_wait call left the cancelable area and is then waiting
     on the mutex.  In this case the beginning of the second cond_wait
     call will cause the cancellation to happen.  */
  do
    if (pthread_cond_wait (&c, &m) != 0)
      {
	//exit (1);
        return NULL;
      }
  while (arg == NULL);
  pthread_cleanup_pop (0);
  if (pthread_mutex_unlock (&m) != 0)
    {
      //exit (1);
      return NULL;
    }
  return NULL;
}


 int cond22 (int cp)
{
uint64_t start = 0;
uint64_t end = 0;
start = rdtsc();
  int status = 0;
  int cpu=1;

  if(cp > 0)
     cpu = cp;

  if (pthread_barrier_init (&b, NULL, 2) != 0)
    {
      puts ("barrier_init failed");
      return 1;
    }
printf("b={%p} m={%p} c={%p} \n",&b,&m,&c);

  pthread_t th;
  if (pthread_create (&th, NULL, tf, (void*)cpu) != 0)
    {
      puts ("1st create failed");
      return 1;
    }
  int e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("1st barrier_wait failed");
      return 1;
    }
  if (pthread_mutex_lock (&m) != 0)
    {
      puts ("1st mutex_lock failed");
      return 1;
    }
  if (pthread_cond_signal (&c) != 0)
    {
      puts ("1st cond_signal failed");
      return 1;
    }
  if (pthread_cancel (th) != 0)
    {
      puts ("cancel failed");
      return 1;
    }
  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("1st mutex_unlock failed");
      return 1;
    }
  void *res;
  if (pthread_join (th, &res) != 0)
    {
      puts ("1st join failed");
      return 1;
    }
  if (res != PTHREAD_CANCELED)
    {
      puts ("first thread not canceled");
      status = 1;
    }

  printf ("cond = { %d, %x, %lld, %lld, %lld, %p, %u, %u }\n",
  	  c.__data.__lock, c.__data.__futex, c.__data.__total_seq,
	  c.__data.__wakeup_seq, c.__data.__woken_seq, c.__data.__mutex,
	  c.__data.__nwaiters, c.__data.__broadcast_seq);

  if (pthread_create (&th, NULL, tf, (void *) 1l) != 0)
    {
      puts ("2nd create failed");
      return 1;
    }
  e = pthread_barrier_wait (&b);
  if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD)
    {
      puts ("2nd barrier_wait failed");
      return 1;
    }
  if (pthread_mutex_lock (&m) != 0)
    {
      puts ("2nd mutex_lock failed");
      return 1;
    }
  if (pthread_cond_signal (&c) != 0)
    {
      puts ("2nd cond_signal failed");
      return 1;
    }
  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("2nd mutex_unlock failed");
      return 1;
    }
  if (pthread_join (th, &res) != 0)
    {
      puts ("2nd join failed");
      return 1;
    }
  if (res != NULL)
    {
      puts ("2nd thread canceled");
      status = 1;
    }

  printf ("cond = { %d, %x, %lld, %lld, %lld, %p, %u, %u }\n",
  	  c.__data.__lock, c.__data.__futex, c.__data.__total_seq,
	  c.__data.__wakeup_seq, c.__data.__woken_seq, c.__data.__mutex,
	  c.__data.__nwaiters, c.__data.__broadcast_seq);

end = rdtsc();
//printf("compute time {%ld} \n",(end-start));
  return status;
}