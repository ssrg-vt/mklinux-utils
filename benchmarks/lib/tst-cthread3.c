 
/*
 * cthread tests program
 *
 * Copyright Antonio Barbalace, SSRG, VT, 2013
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>

#include "cthread.h"

#if 1
// CHILD DIES FIRST
 #define PARENT_SLEEP 7
 #define CHILD_SLEEP 3
#else
// PARENT DIES FIRST
 #define PARENT_SLEEP 5
 #define CHILD_SLEEP 15
#endif

#define SLEEP_FACT 1000000
#define _sleep(a) { int i; long aa=a; long bb=-3; for (i=0; i<(a*SLEEP_FACT); i++) aa += (bb + a);}
#define _sleep sleep

void * fun(void * arg)
{
  int id = (int)arg;

  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(id, &cpu_mask);

  printf("in fun\n"); 
  cthread_setaffinity_np(0, sizeof(cpu_set_t), &cpu_mask)  ; // NOTE first argument is not pthread or cthread
  system("echo \"cthread_test3: setaffinity\" > /dev/kmsg\n");

  _sleep(CHILD_SLEEP);
  system("echo \"cthread_test3: out fun\" > /dev/kmsg\n");
//  printf("out fun\n"); //printf requires futex
  return 0;
}

int main (int argc, char * argv[])
{
  cthread_t pt;
  int res;
  int cpuid = 1;

  errno = 0;
  if (argc >= 2)
    if ( (cpuid = strtol(argv[1], 0, 10)) == 0 )
      if (errno == EINVAL || errno == ERANGE)
       cpuid = 1;
 
  unsigned long saved_context = cthread_initialize();

  printf("pre pthread\n");
  res = cthread_create(&pt, 0, fun, (void*) cpuid);
printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  _sleep(PARENT_SLEEP);
  printf("out main\n");

  printf("pre pthread\n");
  res = cthread_create(&pt, 0, fun, (void*) cpuid);
printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  _sleep(PARENT_SLEEP);
  printf("out main\n");

  cthread_restore(saved_context);

  return 0;
}

/*
void test_modula() {
	cthread_create();
	cthread_join();

	cthread_create();
	cthread_join();

	cthread_create();
	cthread_join(); // in which way is done in OpenMP ?!
}

int main () {
  test_modula();
}
*/
