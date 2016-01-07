/*
 * racey-tcp-server-epoll.c
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
#include <sys/syscall.h>
#include <sys/epoll.h>

#define OUTPUT_FILE "./output.txt"
#define MAX_EVENTS 2048

int main(int argc, char **argv)
{
	struct sockaddr_in serv_addr;    /* Local address */
    struct sockaddr_in client_addr;  /* Client address */
	unsigned short server_port;      /* Server port */
	unsigned int client_len;         /* Length of client address data structure */
	int server_fd;
	int client_fd;
	int output_fd;
	int i;
	int efd;
	struct epoll_event e;
	struct epoll_event *events;
	int flags;

	if (argc != 2)     /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
		exit(1);
	}

	server_port = atoi(argv[1]);  /* First arg:  local port */

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

	/* Must make the listening socket to be non-blocking */
	if (flags = fcntl(server_fd, F_GETFL, 0) == -1) {
		perror("fcntl error");
		return 1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(server_fd, F_SETFL, flags) == -1) {
		perror("fcntl error");
		return 1;
	}

	if (listen(server_fd, 10) < 0) {
		return 1;
	}

	efd = epoll_create1(0); /* Create epoll fd */
	if (efd == -1) {
		perror("epoll_create error");
		return 1;
	}

	e.data.fd = server_fd;   /* Register the listening socket */
	e.events = EPOLLIN | EPOLLET;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, server_fd, &e) == -1) {
		perror("epoll_ctl error");
		return 1;
	}

	events = malloc(MAX_EVENTS * sizeof(struct epoll_event));

	client_len = sizeof(client_addr);
	printf("server all ready\n");
	for (;;) {
		int n, i;

		printf("waiting\n");
		n = epoll_wait(efd, events, MAX_EVENTS, -1);
		for (i = 0; i < n; i++) {
			if ((events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) ||
				(!(events[i].events & EPOLLIN))) {
				/* The epoll has gone wrong */
				printf("An unusual socket %d\n", events[i].data.fd);
				close(events[i].data.fd);
			} else if (events[i].data.fd == server_fd) { /* The listening socket is ready */
				printf("accept %d\n", events[i].data.fd);
				if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr,
								&client_len)) < 0) {
					return 1;
				}

				/* Must make the socket to be non-blocking */
				if (flags = fcntl(client_fd, F_GETFL, 0) == -1) {
					perror("fcntl error");
					return 1;
				}

				flags |= O_NONBLOCK;
				if (fcntl(client_fd, F_SETFL, flags) == -1) {
					perror("fcntl error");
					return 1;
				}

				/* Register the client socket */
				e.data.fd = client_fd;
				e.events = EPOLLIN | EPOLLET;
				if (epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &e) == -1) {
					perror("epoll_ctl error");
					return 1;
				}
			} else {
				/* Finally a socket is ready to read */
				int nr = 0;
				int fd;
				char buf[256];
				fd = events[i].data.fd;
				do {
					nr = read(fd, buf, sizeof(buf));
					printf("read %d from %d\n", nr, fd);
					write(output_fd, buf, nr);
				} while (nr > 0);
				close(fd);
			}
		}
	}

	free(events);
	close(server_fd);
	return 0;
}
