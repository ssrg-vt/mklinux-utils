/* test_kmsg.c is used to test inter-kernel messaging */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __NR_popcorn_test_kmsg 314

typedef unsigned long pcn_kmsg_mcast_id;

enum pcn_kmsg_test_op {
	PCN_KMSG_TEST_SEND_SINGLE,
	PCN_KMSG_TEST_SEND_BATCH,
	PCN_KMSG_TEST_SEND_LONG,
	PCN_KMSG_TEST_OP_MCAST_OPEN,
	PCN_KMSG_TEST_OP_MCAST_SEND,
	PCN_KMSG_TEST_OP_MCAST_CLOSE
};

struct pcn_kmsg_test_args {
	int cpu;
	unsigned long mask;
	pcn_kmsg_mcast_id mcast_id;
};

int main(int argc,  char *argv[]) 
{
	int opt;
	int test_op, rc;
	struct pcn_kmsg_test_args test_args;

	test_args.cpu = 0;

	while ((opt = getopt(argc, argv, "c:t:")) != -1) {
		switch (opt) {
			case 'c':
				test_args.cpu = atoi(optarg);
				break;

			case 't':
				test_op = atoi(optarg);
				break;

			default:
				fprintf(stderr, "Usage: %s [-c cpu] [-t test_op]\n",
					argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	printf("pcn_kmsg test syscall, cpu %d, test_op %d...\n", 
	       test_args.cpu, test_op);

	rc = syscall(__NR_popcorn_test_kmsg, test_op, &test_args);

	printf("Syscall returned %d\n", rc);

	exit(EXIT_SUCCESS);
}
