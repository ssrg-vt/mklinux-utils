#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syscall.h"

#define N 5000

static char *shm;

static inline unsigned long rdtscll(void)
{
	unsigned int a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return ((unsigned long) a) | (((unsigned long) d) << 32);
}

static unsigned long calculate_tsc_overhead(void)
{
	unsigned long t0, t1, overhead = ~0UL;
	int i;

	for (i = 0; i < N; i++) {
		t0 = rdtscll();
		asm volatile("");
		t1 = rdtscll();
		if (t1 - t0 < overhead)
			overhead = t1 - t0;
	}

	printf("tsc overhead is %ld\n", overhead);

	return overhead;
}

static void run_test(void)
{
	unsigned long tsc, result;
	unsigned long overhead = calculate_tsc_overhead();
	int i;
	volatile struct syscall_packet *pkt = (struct syscall_packet *) shm;

	for (i = 0; i < N; i++) {
		tsc = rdtscll();

		pkt->regs[REG_RAX] = 1;
		pkt->regs[REG_RDI] = 10;
		pkt->regs[REG_RSI] = 20;
		barrier();
		set_ready(pkt);

		while (is_ready(pkt))
			cpu_relax();

		if (pkt->regs[REG_RAX] != 30) {
			printf("message send failed\n");
			exit(1);
		}

		result = rdtscll() - tsc - overhead;
		if (result < 10000)
			printf("%ld\n", result);
	}
}

int main(int argc, char *argv[])
{
	int shmid;
	key_t key;
	
	key = 5679;

	if ((shmid = shmget(key, 4096, 0666)) < 0) {
		perror("shmget");
		exit(1);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		exit(1);
	}

	run_test();

	return 0;
}
