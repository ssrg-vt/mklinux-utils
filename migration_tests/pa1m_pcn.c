#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>


#define L3_SIZE 6291456 //6144 kB
#define CACHE_LINE 64 //64 B
#define PAD_SIZE CACHE_LINE-4 //CACHE_LINE-sizeof(int)
#define ARRAY_SIZE 2*(L3_SIZE/CACHE_LINE)
//#define ARRAY_SIZE 100

typedef struct padded_int_value{

	int value;
	char pad[PAD_SIZE];

}pad_int;

int _dst;
int iterations;
int coordination;
unsigned long *times;
unsigned long *msgs;
unsigned long *ipis;
pad_int * array;
//char ___log[]="sto uscendo\n";
char buffer[1024];

int cfn( void*blah ) {
   
    cpu_set_t cpu_mask;
    int i,j,app;
    pad_int* scorre;
    long* scorre2;
    struct timeval start;
    struct timeval end;
    unsigned long mget, mput, dummy, ipi;
int fd;


    printf("Migrating... was on %d\n", sched_getcpu());
    CPU_ZERO(&cpu_mask);
    CPU_SET(_dst,&cpu_mask);

    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_mask);
    while ( sched_getcpu() != _dst);


    scorre2= times;
    for(i=0;i<iterations;i++){
        scorre=array;
	
	gettimeofday(&start,NULL);
	for(j=0;j<ARRAY_SIZE;j++){
		app=scorre->value;
		scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));
	}
	
	gettimeofday(&end,NULL);
	*scorre2= (1000000 *end.tv_sec+ end.tv_usec)-(1000000 *start.tv_sec+ start.tv_usec);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
	
	fd =open ("/proc/pcnmsg", O_RDWR);
	read(fd, buffer, 1024);
	sscanf(buffer, "msg_get: %ld failed: %ld \nmsg_put: %ld failed: %ld \n"
		"msg_mput: %ld failed: %ld \nmsg_mput: %ld failed: %ld\n"
		"ipi: %ld suppressed: %ld\n",
		&mget, &dummy, &mput, &dummy,
		&dummy, &dummy, &dummy, &dummy,
		&ipi, &dummy);
//	write (fd, buffer,1);
	close(fd);
	msgs[i] = mget + mput;
	ipis[i] = ipi;
}

    coordination=1;
//system(___log);  
    while(1){}
    syscall(__NR_exit);
    


}


int main( int argc, char**argv ) {
    int i;
    long tot;
    pad_int* scorre;
    long* scorre2;
    const int STACK_SZ = 65536;

printf ("sizeof %d ARRAY %d total %d\n", sizeof (pad_int), ARRAY_SIZE, ARRAY_SIZE* sizeof(pad_int));

    if(argc == 3) {
        _dst = atoi(argv[1]);
	iterations = atoi(argv[2]);
    }
    else{
 	printf("arg: dest_cpu number_iterations\n");
        exit(0);
    }

    if( _dst == sched_getcpu()) {
        printf("dst == current, exiting\n");
        exit(0);
    }

    if( iterations <=0 ) {
        printf("iterations <=0 , exiting\n");
        exit(0);
    }

    array= (pad_int*)malloc(ARRAY_SIZE*sizeof(pad_int));
   
    if(!array){
 	printf("Unable to allocate array\r\n");
        return 0;
    }

msgs= (long*)malloc(iterations*sizeof(long));

    if(!msgs){
        printf("Unable to allocate time msgs\r\n");
        return 0;
    }

ipis= (long*)malloc(iterations*sizeof(long));

    if(!ipis){
        printf("Unable to allocate time ipis\r\n");
        return 0;
    }
 
    times= (long*)malloc(iterations*sizeof(long));
   
    if(!times){
 	printf("Unable to allocate time array\r\n");
        return 0;
    }

    scorre=array;
    for(i=0;i<ARRAY_SIZE;i++){
	scorre->value=0;
	scorre=(pad_int*) ((void*)scorre+sizeof(pad_int));

    }

    void* stack = malloc(STACK_SZ);
    if(!stack) {
        printf("Unable to allocate stack\r\n");
        return 0;
    }
	
    coordination=0;
	
    printf("Cloning...\n");
 
    clone(cfn, stack+STACK_SZ, CLONE_THREAD|CLONE_SIGHAND|CLONE_VM, NULL);
	
    while(coordination==0){}
	
    tot=0;
    scorre2=times;
    for(i=0;i<iterations;i++){
	printf("iteration %i: %ld %ld %ld\n",i,(*scorre2), msgs[i], ipis[i]);
	tot=tot+(*scorre2);
	scorre2=(long*) ((void*)scorre2+sizeof(long));
    }
	
    printf("total: %ld\n",tot);
    
    //while(1){} 
    return 0;
}
