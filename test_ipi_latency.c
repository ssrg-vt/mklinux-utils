/* test_ipi_latency.c is used to test inter-kernel IPI latency */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __NR_popcorn_test_ipi_latency 315

int main(int argc,  char *argv[]) 
{
	int cpu, rc;

	if (argc != 2) {
		printf("Invalid number of arguments specified!\n");
		return 0;
	}

	sscanf(argv[1], "%d", &cpu);

	printf("Sending test message to CPU %d...\n", cpu);

	rc = syscall(__NR_popcorn_test_ipi_latency, cpu);

	printf("Syscall returned %d\n", rc);

	return 0;
}
