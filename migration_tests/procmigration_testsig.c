#define _GNU_SOURCE
#include <sched.h>  
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define POPCORN

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}

 static void
 sigsegvHandler(int sig)
 {
     int x;

     /* UNSAFE: This handler uses non-async-signal-safe functions
        (printf(), strsignal(), fflush(); see Section 21.1.2) */

     printf("Caught signal %d (%s)\n", sig, strsignal(sig));
     printf("Top of handler stack near     %10p\n", (void *) &x);
     fflush(NULL);

     _exit(1);                /* Can't return after SIGSEGV */
 }


int main()
{

 cpu_set_t mas;
 int ret;
 CPU_ZERO(&mas);
 CPU_SET((int)3, &mas);

  int __ret;
  pid_t pid;
  int cpu_src = sched_getcpu();
  char buf[512];
  if (cpu_src == -1)
    perror("sched_getcpu");
  
  int total_cpus = sysconf(_SC_NPROCESSORS_CONF) - 1;
#ifdef POPCORN
  int cpu_dest =3;
#else
  int cpu_dest = (cpu_src +1) % total_cpus;
#endif
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu_dest, &cpu_mask);

     struct sigaction sa;
   
     sa.sa_handler = sigsegvHandler;
     sigemptyset(&sa.sa_mask);
     sigaddset(&sa.sa_mask,SIGALRM);
     sa.sa_flags = SA_ONSTACK;           /* Handler uses alternate stack */
     if (sigaction(SIGPIPE, &sa, NULL) == -1)
         _exit("sigaction");
     if (sigaction(SIGILL, &sa, NULL) == -1)
         _exit("sigaction");
  
  uint64_t start = rdtsc();
  uint64_t end = 0;
  pid = fork();
  if ( !pid ) {
       
     stack_t sigstack;
     int j;

     sigstack.ss_sp = malloc(SIGSTKSZ);
     if (sigstack.ss_sp == NULL)
        _exit("malloc");

     sigstack.ss_size = SIGSTKSZ;
     sigstack.ss_flags = 0;
     if (sigaltstack(&sigstack, NULL) == -1)
         _exit("sigaltstack");
     printf("Alternate stack is at         %10p-%p\n",
             sigstack.ss_sp, (char *) sbrk(0) - 1);

     sigset_t signal_set;
     sigemptyset(&signal_set);

     sigaddset(&signal_set, SIGSEGV);
     sigaddset(&signal_set, SIGINT);


     sigprocmask(SIG_BLOCK, &signal_set, NULL);
     printf("Top of standard stack is near %10p\n", (void *) &j);

    printf("task pid %d\n",getpid());
    __ret = sched_setaffinity( 0, sizeof(cpu_set_t), &cpu_mask);
    while(1) {};
    end = rdtsc(); 
    sprintf(buf,"echo child: migration time{%ld} start{%ld} end{%ld} pid{%d} > /dev/kmsg",
            (end - start), start, end, getpid());
    system(buf);
    return 0;
  }
  else
    printf("pids child: %d parent: %d\n",
      pid, getpid());

  return 0;
}

