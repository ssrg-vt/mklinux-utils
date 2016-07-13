#include <unistd.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "popcorn_threadpool.h"

int main(int argc, char* argv[]){

	long l = sysconf(_SC_NPROCESSORS_ONLN);
	printf("_SC_NPROCESSORS: %ld\n",l);
	return 0;
}
