/*
 * cthread tests program
 *
 * Copyright Antonio Barbalace, SSRG, VT, 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

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

void * fun(void * arg)
{
  printf("in fun\n");
  sleep(CHILD_SLEEP);
  printf("out fun\n");
  return 0;
}

int main (int argc, char * argv[])
{
  cthread_t pt;
  int res;

  unsigned long saved_context = cthread_initialize();

  printf("pre pthread\n");
  res = cthread_create(&pt, 0, fun, 0);
printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  sleep(PARENT_SLEEP);
  printf("out main\n");

  printf("pre pthread\n");
  res = cthread_create(&pt, 0, fun, 0);
printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  sleep(PARENT_SLEEP);
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
