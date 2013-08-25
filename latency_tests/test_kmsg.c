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
	PCN_KMSG_TEST_SEND_PINGPONG,
	PCN_KMSG_TEST_SEND_BATCH,
	PCN_KMSG_TEST_SEND_BATCH_RESULT,
	PCN_KMSG_TEST_SEND_LONG,
	PCN_KMSG_TEST_OP_MCAST_OPEN,
	PCN_KMSG_TEST_OP_MCAST_SEND,
	PCN_KMSG_TEST_OP_MCAST_CLOSE,
	PCN_KMSG_TEST_CPU_WAIT,
	PCN_KMSG_TRIGGER_CPU_WAIT_IDLE,
	PCN_KMSG_TRIGGER_CPU_WAIT_SCHED,
	PCN_KMSG_TRIGGER_CPU_WAIT_SCHED_IDLE,
	PCN_KMSG_TRIGGER_CPU_WAIT
};

struct pcn_kmsg_test_args {
	int cpu;
	unsigned long mask;
	unsigned long batch_size;
	pcn_kmsg_mcast_id mcast_id;
	unsigned long send_ts;
	unsigned long ts0;
	unsigned long ts1;
	unsigned long ts2;
	unsigned long ts3;
	unsigned long ts4;
	unsigned long ts5;
	unsigned long rtt;
};

void print_usage(void)
{
	fprintf(stderr, 
		"Usage: test_kmsg [-c cpu] [-t test_op] [-b batch_size]\n"
		"[-n num_tests] [-i mcast_id]\n");
        fprintf(stderr," Test Ops :- \n");
        fprintf(stderr,"PCN_KMSG_TEST_SEND_SINGLE %d \n",
		PCN_KMSG_TEST_SEND_SINGLE);
	fprintf(stderr,"PCN_KMSG_TEST_SEND_PINGPONG %d \n",
		PCN_KMSG_TEST_SEND_PINGPONG);
	fprintf(stderr,"PCN_KMSG_TEST_SEND_BATCH %d \n",
		PCN_KMSG_TEST_SEND_BATCH);
	fprintf(stderr,"PCN_KMSG_TEST_SEND_BATCH_RESULT %d \n",	
		PCN_KMSG_TEST_SEND_BATCH_RESULT);
	fprintf(stderr,"PCN_KMSG_TEST_SEND_LONG %d \n",	
		PCN_KMSG_TEST_SEND_LONG);
	fprintf(stderr,"PCN_KMSG_TEST_OP_MCAST_OPEN %d \n",
		PCN_KMSG_TEST_OP_MCAST_OPEN);
	fprintf(stderr,"PCN_KMSG_TEST_OP_MCAST_SEND %d \n",
		PCN_KMSG_TEST_OP_MCAST_SEND);
	fprintf(stderr,"PCN_KMSG_TEST_OP_MCAST_CLOSE %d \n",
		PCN_KMSG_TEST_OP_MCAST_CLOSE);
	fprintf(stderr,"PCN_KMSG_TEST_CPU_WAIT %d \n",
		PCN_KMSG_TEST_CPU_WAIT);
	fprintf(stderr,"PCN_KMSG_TRIGGER_CPU_WAIT_IDLE %d \n",
		PCN_KMSG_TRIGGER_CPU_WAIT_IDLE);
	fprintf(stderr,"PCN_KMSG_TRIGGER_CPU_WAIT_SCHED %d \n",
		PCN_KMSG_TRIGGER_CPU_WAIT_SCHED);
	fprintf(stderr,"PCN_KMSG_TRIGGER_CPU_WAIT_SCHED_IDLE %d \n",
		PCN_KMSG_TRIGGER_CPU_WAIT_SCHED_IDLE);
	fprintf(stderr,"PCN_KMSG_TRIGGER_CPU_WAIT %d \n",
		PCN_KMSG_TRIGGER_CPU_WAIT);
}

int main(int argc,  char *argv[]) 
{
	int opt, i;
	int test_op, rc;
	struct pcn_kmsg_test_args test_args;
	unsigned long num_tests = 1;

	test_args.cpu = -1;
	test_args.mask = 0;
	test_args.mcast_id = -1;
	test_args.batch_size = 1;
	test_op = -1;

	while ((opt = getopt(argc, argv, "hc:t:b:n:m:i:")) != -1) {
		switch (opt) {
			case 'h':
			        print_usage();
				break;
			case 'c':
				test_args.cpu = atoi(optarg);
				break;

			case 't':
				test_op = atoi(optarg);
				break;

			case 'b':
				test_args.batch_size = atoi(optarg);
				break;

			case 'n':
				num_tests = atoi(optarg);
				break;

			case 'm':
				test_args.mask = strtoul(optarg, NULL, 0);
				break;

			case 'i':
				test_args.mcast_id = atoi(optarg);
				break;

			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}

	if (test_args.cpu == -1) {
		fprintf(stderr, "Failed to specify CPU!\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (test_op == -1) {
		fprintf(stderr, "Failed to specify test operation!\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	if ((test_op == PCN_KMSG_TEST_OP_MCAST_OPEN) && !test_args.mask) {
		fprintf(stderr, "Failed to specify mask for mcast open!\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	if ((test_op == PCN_KMSG_TEST_OP_MCAST_SEND) && (test_args.mask == -1)) {
		fprintf(stderr, "Failed to specify group id for mcast send!\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	printf("pcn_kmsg test syscall, cpu %d, test_op %d...\n", 
	       test_args.cpu, test_op);

	if (test_op == PCN_KMSG_TEST_SEND_PINGPONG) {
		printf("send,IPI,isr1,isr2,bh,bh2,handler,roundtrip\n");
	}

	if ((test_op == PCN_KMSG_TEST_SEND_BATCH) || 
	    (test_op == PCN_KMSG_TEST_SEND_LONG)) {
		printf("send,sendreturn,bh,bh2,lasthandler,roundtrip\n");
	}

	if(test_op == PCN_KMSG_TRIGGER_CPU_WAIT) {
	  printf("Spin START,targ_cpu,rep_cpu,rep_val,send_val,rtt\n");
	}
	if(test_op == PCN_KMSG_TRIGGER_CPU_WAIT_IDLE) {
	  printf("Idle START,targ_cpu,rep_cpu,rep_val,send_val,rtt\n");
	}
	if(test_op == PCN_KMSG_TRIGGER_CPU_WAIT_SCHED) {
	  printf("Sched START,targ_cpu,rep_cpu,rep_val,send_val,rtt\n");
	}
	if(test_op == PCN_KMSG_TRIGGER_CPU_WAIT_SCHED_IDLE) {
	  printf("Sched_Idle,targ_cpu,rep_cpu,rep_val,send_val,rtt\n");
	}



	for (i = 0; i < num_tests; i++) {

		if ((num_tests > 1 )
                        && (test_op == PCN_KMSG_TRIGGER_CPU_WAIT)) {
			rc = syscall(__NR_popcorn_test_kmsg,
	                               PCN_KMSG_TEST_CPU_WAIT,
	                               &test_args);
		}

		rc = syscall(__NR_popcorn_test_kmsg, test_op, &test_args);
		if (rc) {
			printf("ERROR: Syscall returned %d\n", rc);
		}

		switch (test_op) {
			case PCN_KMSG_TRIGGER_CPU_WAIT:
			case PCN_KMSG_TRIGGER_CPU_WAIT_IDLE:
			case PCN_KMSG_TRIGGER_CPU_WAIT_SCHED:		
			case PCN_KMSG_TRIGGER_CPU_WAIT_SCHED_IDLE:
			        printf("%010lu,%02d,%02d,%08x,%08x,%lu\n",
				       test_args.send_ts,
				       (int)test_args.ts0,
				       (int)test_args.ts1,
				       (int)test_args.ts2, 
				       (int)test_args.ts3,
				       test_args.rtt);
				break;

			case PCN_KMSG_TEST_SEND_SINGLE:
				printf("Single ticks: sender %lu\n",
				       test_args.send_ts);

			case PCN_KMSG_TEST_SEND_PINGPONG:
				printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", 
				       test_args.send_ts, 
				       test_args.ts0-test_args.send_ts, 
				       test_args.ts1-test_args.send_ts, 
				       test_args.ts2-test_args.send_ts, 
				       test_args.ts3-test_args.send_ts, 
				       test_args.ts4-test_args.send_ts, 
				       test_args.ts5-test_args.send_ts, 
				       test_args.rtt-test_args.send_ts);
				break;

			case PCN_KMSG_TEST_SEND_LONG:
			case PCN_KMSG_TEST_SEND_BATCH:
				printf("%lu,%lu,%lu,%lu,%lu,%lu\n",
				       test_args.send_ts, test_args.ts0, test_args.ts1,
				       test_args.ts2, test_args.ts3, test_args.rtt);
				break;

			//case PCN_KMSG_TEST_SEND_LONG:
			//	printf("Long ticks: sender %lu\n",
			//	       test_args.send_ts);
			//	break;

			case PCN_KMSG_TEST_OP_MCAST_OPEN:
				printf("Opened mcast group, ID %lu\n",
				       test_args.mcast_id);
				break;

			case PCN_KMSG_TEST_OP_MCAST_SEND:
				printf("Sent message to mcast group %lu, sender %lu, receiver %lu\n",
				       test_args.mcast_id, test_args.send_ts, test_args.ts0);
				break;
		}
		usleep(20000);
	}

	exit(EXIT_SUCCESS);
}
