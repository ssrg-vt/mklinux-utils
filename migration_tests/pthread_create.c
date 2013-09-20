//antonio barbalace 2013

#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <asm/ldt.h>

#if 1
//#if 0
// CHILD DIES FIRST
 #define PARENT_SLEEP 15
 #define CHILD_SLEEP 5
#else
// PARENT DIES FIRSTs
 #define PARENT_SLEEP 5
 #define CHILD_SLEEP 15
#endif

int fun(void * arg)
{
  printf("in fun\n");
  sleep(CHILD_SLEEP);
  printf("out fun\n");
  syscall(__NR_exit); // <--- this will emulate 100% pthread
  //_exit(0);
  //return 0;
}

//from glibc
#define CLONE_SIGNAL		(CLONE_SIGHAND | CLONE_THREAD)
#define STACK_SIZE 		0x1000

int main (int argc, char * argv[])
{ 
  //from glibc
  int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGNAL
		     //| CLONE_SETTLS | CLONE_PARENT_SETTID
		     | CLONE_PARENT_SETTID
		     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
//#if __ASSUME_NO_CLONE_DETACHED == 0 
//		     | CLONE_DETACHED // <-- in our system this is NOT set
//#endif
		     | 0);
  
  //clone_flags = (CLONE_THREAD|CLONE_SIGHAND|CLONE_VM); //Marina's code
  
  int res;
  pid_t ptid;
  pid_t ctid;
  void * stack = malloc(STACK_SIZE);
  struct user_desc * tls = malloc(sizeof(struct user_desc));
  if (!stack) {
    printf("stack mallo error\n");
    return 1;
  }
  if (!tls) {
    printf("tls malloc error\n");
    return 1;
  }

  printf("pre clone\n");
  res = clone(fun, (stack + STACK_SIZE),
	      clone_flags, 0, 
	      &ptid, tls, &ctid);
  if (res == -1) {
    perror("clone");
    return 1;
  }
  printf("after clone\n");
  sleep(PARENT_SLEEP);
  printf("out main\n");
  return 0;
}

