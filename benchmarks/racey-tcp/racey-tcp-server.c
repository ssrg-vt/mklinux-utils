/*
 * racey-tcp-server.c
 * Copyright (C) 2015 Yuzhong Wen <wyz2014@vt.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>

#define WORKER_NUM 5
#define OUTPUT_FILE "./output.txt"

int client_fds[WORKER_NUM];
int client_idx;
int output_fd;
int barrier;
pthread_mutex_t client_fds_lock;
pthread_mutex_t file_lock;

void* racey_worker(void* data)
{
	int fd;
	int fault_count = 0;
	int n = 0;
	char buf[256];
	int tid = *(int *)data;

	while (barrier == 0) {}

	for (;;) {
		/* Waiting for an availiable socket */
		for (;;) {
			syscall(319);
			pthread_mutex_lock(&client_fds_lock);
			syscall(320);
			if ( client_idx>=0 && client_fds[client_idx] > 0) {
				fd = client_fds[client_idx];
				client_fds[client_idx] = -1;
				client_idx --;
				fault_count ++;
				syscall(319);
				pthread_mutex_unlock(&client_fds_lock);
				syscall(320);
				break;
			}
			syscall(319);
			pthread_mutex_unlock(&client_fds_lock);
			syscall(320);
		}
		syscall(321, 1);
		do {
			syscall(319);
			//n = read(fd, buf, sizeof(buf));
			n = recv(fd, buf + 10, sizeof(buf), 0);
			syscall(320);
			sprintf(buf, "\n%d ", tid);
			syscall(319);
			write(output_fd, buf, n);
			syscall(320);
		} while (n > 0);
		close(fd);
	}
}

int main(int argc, char **argv)
{
	struct sockaddr_in serv_addr;    /* Local address */
    struct sockaddr_in client_addr;  /* Client address */
	unsigned short server_port;      /* Server port */
	unsigned int client_len;         /* Length of client address data structure */
	int server_fd;
	int client_fd;
	int i;
	int tinfo[WORKER_NUM];
	pthread_t threads[WORKER_NUM];
	pthread_attr_t attr;

	if (argc != 2)     /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
		exit(1);
	}

	server_port = atoi(argv[1]);  /* First arg:  local port */

	pthread_mutex_init(&client_fds_lock, NULL);
	pthread_mutex_init(&file_lock, NULL);

	/* Open up the output file, which is supposed to be a mess */
	output_fd = open(OUTPUT_FILE, O_WRONLY|O_CREAT, 0644);
	if (output_fd == -1) {
		return 1;
	}

	/* Create socket for incoming connections */
	if ((server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return 1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));   /* Zero out structure */
	serv_addr.sin_family = AF_INET;                /* Internet address family */
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	serv_addr.sin_port = htons(server_port);      /* Local port */

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		return 1;
	}

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	client_idx = -1;
	barrier = 0;
	for (i = 0; i < WORKER_NUM; i++) {
		client_fds[i] = -1;
		tinfo[i] = i;
		if (pthread_create(&threads[i], &attr, racey_worker, &tinfo[i]) < 0) {
			return 1;
		}
	}
	barrier = 1;

	if (listen(server_fd, 10) < 0) {
		return 1;
	}

	for (;;) {
		/* Accept whatever is coming to me */
		client_len = sizeof(client_addr);
		syscall(321, 2);
		syscall(319);
		if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr,
						&client_len)) < 0) {
			syscall(320);
			return 1;
		}
		syscall(320);
		//syscall(320);

		while (1) {
			syscall(319);
			pthread_mutex_lock(&client_fds_lock);
			syscall(320);
			if (client_idx < WORKER_NUM -1) {
				/* Feed the socket to workers */
				//syscall(320);
				client_idx ++;
				client_fds[client_idx] = client_fd;
				syscall(319);
				pthread_mutex_unlock(&client_fds_lock);
				syscall(320);
				break;
			}
			 syscall(319);
			pthread_mutex_unlock(&client_fds_lock);
			syscall(320);
		}
	}
}
