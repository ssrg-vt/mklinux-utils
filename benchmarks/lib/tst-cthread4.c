 
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
void * stack_prev, * stack_next;

asm volatile ("mov %%rsp, %0\n" : "=m" (stack_prev) : : "memory");
  unsigned long saved_context = cthread_initialize();
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_next) : : "memory");
if (stack_prev != stack_next)
  printf("0: cthread_initialize modified stack pointer %p %p\n", stack_prev, stack_next);

  printf("pre pthread\n");
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_prev) : : "memory"); 
  res = cthread_create(&pt, 0, fun, 0);
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_next) : : "memory");
if (stack_prev != stack_next)
  printf("1: cthread_create modified stack pointer %p %p\n", stack_prev, stack_next);

printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  sleep(PARENT_SLEEP);
  printf("out main\n");

  printf("pre pthread\n");
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_prev) : : "memory");
  res = cthread_create(&pt, 0, fun, 0);
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_next) : : "memory");
if (stack_prev != stack_next)
  printf("2: cthread_create modified stack pointer %p %p\n", stack_prev, stack_next);

printf("cthread_create returned %d\n", res);
//  if (res != 0) {
//    perror("pthread_create");
//    return 1;
//  }
  printf("after pthread\n");
  sleep(PARENT_SLEEP);
  printf("out main\n");

asm volatile ("mov %%rsp, %0\n" : "=m" (stack_prev) : : "memory");
  cthread_restore(saved_context);
asm volatile ("mov %%rsp, %0\n" : "=m" (stack_next) : : "memory");
if (stack_prev != stack_next)
  printf("3: cthread_restore modified stack pointer %p %p\n", stack_prev, stack_next);

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
