#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syscall.h"

static char *shm;

static int64_t add_nums(int64_t a, int64_t b)
{ 
	return a + b;
}

static void run_server(void)
{
	volatile struct syscall_packet *pkt = (struct syscall_packet *) shm;

	while(1) {
		while (!is_ready(pkt))
			cpu_relax();

		if (pkt->regs[REG_RAX] != 1) {
			printf("message receive failed\n");
			exit(1);
		}

		pkt->regs[REG_RAX] = add_nums(pkt->regs[REG_RDI],
				pkt->regs[REG_RSI]);
		barrier();
		clear_ready(pkt);
	}
}

int main(int argc, char *argv[])
{
	int shmid;
	key_t key;

	key = 5679;

	if ((shmid = shmget(key, 4096, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		exit(1);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		exit(1);
	}

	run_server();

	return 0;
}

