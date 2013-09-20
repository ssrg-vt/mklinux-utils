//antonio barbalace 2013

#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <asm/ldt.h>

#include <pthread.h>

//#if 1
#if 0
// CHILD DIES FIRST
 #define PARENT_SLEEP 15
 #define CHILD_SLEEP 5
#else
// PARENT DIES FIRST
 #define PARENT_SLEEP 5
 #define CHILD_SLEEP 15
#endif

void * fun(void * arg)
{
pthread_t ptid = pthread_self();
pthread_attr_t attr;
pthread_getattr_np(ptid, &attr);
size_t stacksize;
pthread_attr_getstacksize(&attr, &stacksize);
printf("fun stacksize %ld\n", stacksize);

  printf("in fun\n");
  sleep(CHILD_SLEEP);
  printf("out fun\n");
  return 0;
}

#define STACK_SIZE 		0x1000

struct pthread_attr
{
  /* Scheduler parameters and priority.  */
  struct sched_param schedparam;
  int schedpolicy;
  /* Various flags like detachstate, scope, etc.  */
  int flags;
  /* Size of guard area.  */
  size_t guardsize;
  /* Stack handling.  */
  void *stackaddr;
  size_t stacksize;
  /* Affinity map.  */
  cpu_set_t *cpuset;
  size_t cpusetsize;
};

int main (int argc, char * argv[])
{ 
  pthread_t pt;
  pthread_attr_t attr;
  int res;

           res = pthread_attr_init(&attr);
           if (res != 0)
               perror("pthread_attr_init");

  struct pthread_attr * iattr = (struct pthread_attr *) &attr;
  printf("guardsize %ld addr %p size %ld\n ", iattr->guardsize, iattr->stackaddr, iattr->stacksize);

  printf("pre pthread\n");
  res = pthread_create(&pt, &attr, fun, 0);

  if (res != 0) {
    perror("pthread_create");
    return 1;
  }
  printf("after pthread\n");
  sleep(PARENT_SLEEP);
  printf("out main\n");
  return 0;
}

