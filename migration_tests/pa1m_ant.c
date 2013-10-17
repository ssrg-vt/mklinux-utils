
// this version, Antonio Barbalace
// previous version, Marina Sadini

// NOTE when you are allocating VM_FILE too there is a segfault

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#ifndef L3_SIZE
  // L3 size is 6144 kB on gigi?!
  #define L3_SIZE	6291456 
#endif
#define CACHE_LINE	64

#ifndef ARRAY_FACTOR
  #define ARRAY_FACTOR  2  
#endif
#ifndef ARRAY_SIZE
  #define ARRAY_SIZE    ARRAY_FACTOR*(L3_SIZE/CACHE_LINE)
#endif
#define ARRAY_VALUE     0x12345678

// selected by the script
/*
#define USE_GETTIMEOFDAY
//#undef USE_GETTIMEOFDAY

//#define MAIN_ALLOCATED_TIMES 
#undef MAIN_ALLOCATED_TIMES

#define MARINA
//#undef MARINA
*/

typedef union _pad_int{
  int value;
  char pad[CACHE_LINE];
}pad_int;

// global variables 
static int _src, _dst;
static int iterations;

volatile static int coordination;

static unsigned long *times;
static pad_int * array;

static char buffer[256];
static char sbuf[256];

static inline unsigned long rdtsc(void)
{
    unsigned int eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((unsigned long)edx << 32) | eax;
}

int cfn( void*blah )
{
    cpu_set_t cpu_mask;
    register int i, j, app, oper;
#ifdef USE_GETTIMEOFDAY    
    struct timeval start, end;
#else
    register unsigned long start, end;
#endif
    
//-----------------------------------------------------------------------------
    printf("I am on %d, migrating... ", sched_getcpu());
    CPU_ZERO(&cpu_mask);
    CPU_SET(_dst,&cpu_mask);
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);
    while ( sched_getcpu() != _dst);
    printf("now on %d\n", sched_getcpu());

#define TEST
#ifdef TEST    
    {    
    struct timeval start, end;
    unsigned long test_value;
    register unsigned long tstart, tend;
    tstart = rdtsc();
    gettimeofday(&end,NULL);
    test_value = ((1000000 * end.tv_sec) + end.tv_usec)
	        -((1000000 * start.tv_sec)+ start.tv_usec);
    tend = rdtsc();
    printf("rdtsc - end %lu start %lu delta %lu, test_value %lu\n"
	   "gettimeofday - end.sec %lu end.usec %lu start.sec %lu start.usec %lu\n",
	   tend, tstart, (tend - tstart), test_value,
	   end.tv_sec, end.tv_usec, start.tv_sec, start.tv_usec);
    }
#endif    

#ifndef MAIN_ALLOCATED_TIMES    
// allocating such stuff prevent any possible cache communication from/to home
    times = (unsigned long *) malloc(iterations*sizeof(unsigned long));
    if ( !times ) {
 	printf("Unable to allocate time array\r\n");
        return 0;
    }
#endif

//-----------------------------------------------------------------------------    
// test starts here    
    for(i=0; i<iterations; i++){
#ifdef USE_GETTIMEOFDAY
	gettimeofday(&start,NULL); 
#else
	start = rdtsc();
#endif		
	for(j=0; j<ARRAY_SIZE; j++){
		app = array[j].value;
		oper += app;
	}
#ifdef USE_GETTIMEOFDAY
	gettimeofday(&end,NULL);
	times[i] = ((1000000 * end.tv_sec) + end.tv_usec)
		  -((1000000 * start.tv_sec)+ start.tv_usec);
#else
	end = rdtsc();
	times[i] = end - start;
#endif			
	 if ( _src = sched_getcpu() != _dst) {
	   printf("error: _dst %d getcpu %d\n", _dst, _src);
	   exit(-1);
	 } 

	int fd = open("/proc/pcnmsg", O_RDONLY);
	memset(buffer, 0, 256);
	read (fd, buffer, 256);
	close (fd);
	sprintf(sbuf, "echo \"%s\" > /dev/kmsg", buffer);
	system (sbuf);
    }
    
    coordination = 1; //sinchronization variable
    
//    while(1){}
    syscall(__NR_exit);
    return 0;
}

#define STACK_SIZE 0x2000
int main( int argc, char**argv )
{
    int i, ret, clone_flags;;
    unsigned long tot;
    cpu_set_t cpu_mask;

//-----------------------------------------------------------------------------    
#ifdef USE_GETTIMEOFDAY    
    printf ("pa1m(Antonio) test - gettimeofday\n"
#else
    printf ("pa1m(Antonio) test - rdtsc\n"
#endif
	    "---------------------------------\n"
	    "sizeof_entry %lu num_of_entry %d size_of_array %lu\n",
	    sizeof(pad_int), ARRAY_SIZE, ARRAY_SIZE* sizeof(pad_int));

    if(argc == 4) {
      _dst = atoi(argv[1]);
      _dst = atoi(argv[2]);
      iterations = atoi(argv[3]);
    }
    else {
 	printf("arg: src_cpu dest_cpu number_iterations\n");
        exit(0);
    }
    
    if( _dst == _src) {
        printf("dst == src, exiting\n");
        exit(0);
    }
    if( iterations <=0 ) {
        printf("iterations <=0 , exiting\n");
        exit(0);
    }

//-----------------------------------------------------------------------------
// fix the task on the current cpu (src) this is required on SMP Linux only
    CPU_ZERO(&cpu_mask);
    CPU_SET(_src,&cpu_mask);
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);
    while ( sched_getcpu() != _src);

    array= (pad_int*)malloc(ARRAY_SIZE*sizeof(pad_int));
    if(!array){
 	printf("Unable to allocate memory for the array\n");
        return 0;
    }
// write in the array on the current cpu (so it will be allocated locally)  
    for(i=0; i<ARRAY_SIZE; i++)
	array[i].value = ARRAY_VALUE;    
    
    times = 0;
#ifdef MAIN_ALLOCATED_TIMES    
//must be allocated local!!! (it is not necessary, but can trash the caches)
    times = (long*)malloc(iterations*sizeof(long));
    if ( !times ) {
 	printf("Unable to allocate time array\r\n");
        return 0;
    }
#endif

//-----------------------------------------------------------------------------
// NOTE
// it will be great if the stack will be allocated locally to the function (i.e.
// on the same node. This is not really possible with normal Linux!
// I will force registers variables in the function
    void* stack = malloc(STACK_SIZE);
    if (!stack) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }
	
    coordination = 0; //synchronization variable
#ifdef MARINA    
    clone_flags = (CLONE_THREAD | CLONE_SIGHAND | CLONE_VM);
#else
  #define CLONE_SIGNAL (CLONE_SIGHAND | CLONE_THREAD)
    clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGNAL
		     //| CLONE_SETTLS | CLONE_PARENT_SETTID
		     | CLONE_PARENT_SETTID
		     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
//		     | CLONE_DETACHED // <-- in our system this is NOT set
		     | 0);
#endif    

    printf("Cloning...\n");
    ret = clone(cfn, (stack + STACK_SIZE), clone_flags, NULL);
    if ( ret == -1 ) {
        printf("Unable to clone task\r\n");
        return 0;
    }    

//-----------------------------------------------------------------------------
    while ( coordination==0 ) ; //synchronization point (data is ready)
	
    if (times == 0) {
      printf("times was never allocated\r\n");
      return 0;
    }
    
    tot = 0;
    for (i=0; i<iterations; i++) {
	printf("iteration %i: %ld %s\n",
	       i, times[i],
	       (array[i].value == ARRAY_VALUE) ? "ok" : "ERR");
	tot=tot + times[i];
    }
	
    printf("total: %ld\n", tot);
    printf("avg: %ld\n", (tot/iterations));
    
// free the global variables
    if (!times)
      printf ("times never allocated!\n");
    free (times);
    if (!array)
      printf ("array never allocated!\n");
    free (array);

    //while(1){} 
    return 0;
}
