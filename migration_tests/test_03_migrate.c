/****************************
 * Test 03 Migrate
 *
 * DKatz
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>

int _dst = 0;

int cfn( void*blah ) {
    char buf[512] = {0};
    cpu_set_t cpu_mask;
    int current_cpu = sched_getcpu();
    int new_cpu = _dst;


    sprintf(buf,"echo Pre-migration child %d, migrating to %d > /dev/kmsg",getpid(),new_cpu);
    system(buf);

    // Do migration
    CPU_ZERO(&cpu_mask);
    CPU_SET(new_cpu,&cpu_mask);
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);

    //sleep(5);
    sprintf(buf,"echo Child %d > /dev/kmsg",getpid());
    system(buf);
    sleep(10);
    return 0;

}



int main( int argc, char**argv ) {
    char buf[512] = {0};
    int cpu = sched_getcpu();
    printf("Test 03 - Clone pid{%d}\r\n",getpid());
    const int STACK_SZ = 65536;
    int current_cpu = sched_getcpu();

    if(argc > 1) {
        _dst = atoi(argv[1]);
    }

    if(_dst == current_cpu) {
        printf("dst == current, exiting\n");
        exit(0);
    }

    sprintf(buf,"echo Parent %d %d > /dev/kmsg",sched_getcpu(),getpid());
    system(buf);
    printf(buf);
    void* stack = malloc(STACK_SZ);
    if(!stack) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }

    clone(cfn, stack+STACK_SZ, NULL, NULL);

    sleep(10);
        
    return 0;
}
