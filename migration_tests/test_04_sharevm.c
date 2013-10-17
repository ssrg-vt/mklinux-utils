/****************************
 * Test 04 sharevm
 *
 * DKatz
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <err.h>
#include <fcntl.h>

#define SYSCALL_POPCORN_PERF_START 316
#define SYSCALL_POPCORN_PERF_END   317

int _dst = 0;
char* hello = NULL;
char* anon = NULL;
char* anon2;

int cfn( void*blah ) {
    char buf[512] = {0};
    cpu_set_t cpu_mask;
    char* anona = (char*)(*(char**)blah);
    int current_cpu = sched_getcpu();
    int new_cpu = _dst;
    int tries = 10;
    anon2 = 1;
    anon2 = NULL;

    sprintf(buf,"echo Pre-migration child %d, migrating to %d > /dev/kmsg",getpid(),new_cpu);
    system(buf);

    // Do migration
    CPU_ZERO(&cpu_mask);
    CPU_SET(new_cpu,&cpu_mask);
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);
    
    tries = 10;
    
    sprintf(buf,"echo Child %d, %lx=%lx > /dev/kmsg",getpid(),&anon,anon);
    system(buf);
    sleep(10);
    
    /*sprintf(buf,"echo waiting a second for mapping > /dev/kmsg");
    while(!anon && tries > 0) {
        //system(buf);
        tries--;
        //sleep(1);
    }*/

    sprintf(buf,"echo past wait > /dev/kmsg");
    system(buf);

    if(anon) {
        sprintf(buf,"echo anon address set %lx=%s > /dev/kmsg",anon,anon);
        system(buf);
        anon[0] = '4';
        anon[1] = '\0';
        sprintf(buf,"echo anon = %s  > /dev/kmsg",anon);
        system(buf);
    } else {
        system("echo anon address not set > /dev/kmsg");
    }
    // OK, then unmap anon, and verify that the main app seg faults
    //munmap(anon,0x1000);
    //sleep(40);

    system("echo mmaping anon2 > /dev/kmsg");
    anon2 = (char*)mmap(NULL,4, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if(anon2 == MAP_FAILED) {
        system("echo failed mmap anon2 > /dev/kmsg");
    } else {
        anon2[0] = '8';
        anon2[1] = '\0';
        sprintf(buf,"echo anon2{value:%lx}{addr:%lx}=%s > /dev/kmsg",anon2,&anon2,anon2);
        system(buf);
    }

//syscall(__NR_exit);
    return 0;

}



int main( int argc, char**argv ) {
    char buf[512] = {0};
    int cpu = sched_getcpu();
    printf("Test 04 - sharevm pid{%d}\r\n",getpid());
    const int STACK_SZ = 65536;
    int current_cpu = sched_getcpu();
    int child, status;

    // Start performance measurements
//    syscall(SYSCALL_POPCORN_PERF_START);

    if(argc > 1) {
        _dst = atoi(argv[1]);
    }

    if(_dst == current_cpu) {
        printf("dst == current, exiting\n");
        exit(0);
    }

    printf("Parent %d %d %lx\n",sched_getcpu(),getpid(),&anon);
    void* stack = malloc(STACK_SZ);
    if(!stack) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }

    child = clone(cfn, stack+STACK_SZ, CLONE_THREAD|CLONE_SIGHAND|CLONE_VM, &anon);
printf("child id %d\n", child);
    sleep(5);

    anon = (char*)mmap(NULL,4, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    anon[0] = '0';
    anon[1] = '\0';
    if(anon == MAP_FAILED) {
        printf("failed mmap\n");
    } else {
        printf("mmapped anon = %lx\n",anon);
    }

    sleep(10);

    //system("pstree\n");

    printf("anon2{value:%lx}{addr:%lx}\n",anon2,&anon2);
    printf("%s\n",anon2);

    printf("waiting on %d\n",child);

   // wait(child); // <--- this is not correct

    waitpid(child, &status, 0);
    printf("waitpid status: %d\n", status);

//    syscall(SYSCALL_POPCORN_PERF_END);

    return 0;
}
