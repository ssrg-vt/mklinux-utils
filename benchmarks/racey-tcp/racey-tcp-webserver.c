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
#define MAX_BUFF (1024*4)

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

int client_fds[WORKER_NUM];
int client_idx;
int output_fd;
int barrier;
int connection_before_error;
int fault_count = 0;
int have_fd = 0;
pthread_mutex_t client_fds_lock;
pthread_mutex_t file_lock;

#define EOL "\r\n"
#define EOL_SIZE 2

typedef struct {
	char *ext;
	char *mediatype;
} extn;

//Possible media types
extn extensions[] ={
	{"gif", "image/gif" },
	{"txt", "text/plain" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"php", "text/html" },
	{"pdf","application/pdf"},
	{"zip","application/octet-stream"},
	{"rar","application/octet-stream"},
	{0,0} };

/*
   A helper function
 */
void error(const char *msg) {
	perror(msg);
	exit(1);
}

/*
   A helper function
 */
int get_file_size(int fd) {
	struct stat stat_struct;
	if (fstat(fd, &stat_struct) == -1)
		return (1);
	return (int) stat_struct.st_size;
}

/*
   A helper function
 */
int send_new(int fd, char *msg, int len) {
	//int len = strlen(msg);
	int ret;
	
	if(len==0){
		len= strlen(msg);
	}
	syscall(319);
	ret= send(fd, msg, len, 0);
	syscall(320);
	if (ret == -1) {
		printf("Error in send\n");
	}

	return ret;
}

/*
   Handles php requests
 */
void php_cgi(char* script_path, int fd) {
        send_new(fd, "HTTP/1.1 200 OK\n Server: Web Server in C\n Connection: close\n", 0);
        dup2(fd, STDOUT_FILENO);
        char script[500];
        strcpy(script, "SCRIPT_FILENAME=");
        strcat(script, script_path);
        putenv("GATEWAY_INTERFACE=CGI/1.1");
        putenv(script);
        putenv("QUERY_STRING=");
        putenv("REQUEST_METHOD=GET");
        putenv("REDIRECT_STATUS=true");
        putenv("SERVER_PROTOCOL=HTTP/1.1");
        putenv("REMOTE_HOST=127.0.0.1");
        execl("/usr/bin/php-cgi", "php-cgi", NULL);
}

/*
   A helper function: Returns the
   web root location.
 */
char* webroot() {
	// open the file "conf" for reading
	FILE *in = fopen("conf", "rt");
	// read the first line from the file
	char buff[1000];
	fgets(buff, 1000, in);
	// close the stream
	fclose(in);
	char* nl_ptr = strrchr(buff, '\n');
	if (nl_ptr != NULL)
		*nl_ptr = '\0';
	return strdup(buff);
}

/*
   This function recieves the buffer
   until an "End of line(EOL)" byte is recieved
 */
int recv_new(int fd, char *buffer) {
	char *p = buffer; // Use of a pointer to the buffer rather than dealing with the buffer directly
	int eol_matched = 0; // Use to check whether the recieved byte is matched with the buffer byte or not
	int ret;
	syscall(319);	
	ret= recv(fd, p, 1, 0);
	syscall(320);
	while (ret != 0) // Start receiving 1 byte at a time
	{
		if (*p == EOL[eol_matched]) // if the byte matches with the first eol byte that is '\r'
		{
			++eol_matched;
			if (eol_matched == EOL_SIZE) // if both the bytes matches with the EOL
			{
				*(p + 1 - EOL_SIZE) = '\0'; // End the string
				return (strlen(buffer)); // Return the bytes recieved
			}
		} else {
			eol_matched = 0;
		}
		p++; // Increment the pointer to receive next byte
		syscall(319);
		ret= recv(fd, p, 1, 0);
		syscall(320);
	}
	return (0);
}

/*
   This function parses the HTTP requests,
   arrange resource locations,
   check for supported media types,
   serves files in a web root,
   sends the HTTP error codes.
 */
int connection(int fd, char* buff) {
	char request[500], resource[500], *ptr;
	int fd1,length;

	if (recv_new(fd, request) == 0) {
		printf("Recieve Failed\n");
	}

	printf("%s\n", request);

	// Check for a valid browser request
	ptr = strstr(request, " HTTP/");
	if (ptr == NULL) {
		printf("NOT HTTP !\n\n");
	} else {
		*ptr = 0;
		ptr = NULL;

		if (strncmp(request, "GET ", 4) == 0) {
			ptr = request + 4;
		}
		if (ptr == NULL) {
			printf("Unknown Request ! \n\n");
		} else {
			if (ptr[strlen(ptr) - 1] == '/') {
				strcat(ptr, "index.html");
			}
			//strcpy(resource, webroot());
			strcpy(resource, "/root/web/");
			strcat(resource, ptr);
			char* s = strchr(ptr, '.');
			int i;
			for (i = 0; extensions[i].ext != NULL; i++) {
				if (strcmp(s + 1, extensions[i].ext) == 0) {

					fd1 = open(resource, O_RDONLY, 0);
					//printf("Opening \"%s\"\n", resource);
					if (fd1 == -1) {
						printf("404 File not found Error\n\n");
						send_new(fd, "HTTP/1.1 404 Not Found\r\n", 0);
						send_new(fd, "Server : Web Server in C\r\n\r\n", 0);
						send_new(fd, "<html><head><title>404 Not Found</head></title>", 0);
						send_new(fd, "<body><p>404 Not Found: The requested resource could not be found!</p></body></html>", 0);
						goto out;
						//Handling php requests
					} else if (strcmp(extensions[i].ext, "php") == 0) {
						printf("Seeee php. Don't try...\n");
						//php_cgi(resource, fd);
						//sleep(1);

					} else {
						//printf("200 OK, Content-Type: %s\n\n",extensions[i].mediatype);
						send_new(fd, "HTTP/1.1 200 OK\r\n", 0);
						send_new(fd, "Server : Web Server in C\r\n\r\n", 0);
						if (ptr == request + 4) // if it is a GET request
						{
							if ((length = get_file_size(fd1)) == -1)
								printf("Error in getting size !\n");
							int total_bytes_sent = 0;
							int bytes_sent;
							int bytes_read;
							while (total_bytes_sent < length) {
								
								syscall(319);
								bytes_read= read(fd1, buff, MAX_BUFF);
								syscall(320);
								if(bytes_read > 0){
									//if ((bytes_sent = sendfile(fd, fd1, 0,
									//				length - total_bytes_sent)) <= 0) {
									
									if((bytes_sent = send_new(fd, buff, bytes_read)) <= 0){
										printf("send file error\n");
										break;	
									}
									total_bytes_sent += bytes_sent;
								}
								else{
									break;
								}
							}

						}
					}	
					syscall(319);
					close(fd1);
					syscall(320);
					goto out;
				}
				int size = sizeof(extensions) / sizeof(extensions[0]);
				if (i == size - 2) {
					printf("415 Unsupported Media Type\n\n");
					send_new(fd, "HTTP/1.1 415 Unsupported Media Type\r\n", 0);
					send_new(fd, "Server : Web Server in C\r\n\r\n", 0);
					send_new(fd, "<html><head><title>415 Unsupported Media Type</head></title>", 0);
					send_new(fd, "<body><p>415 Unsupported Media Type!</p></body></html>", 0);
				}
			}

			//close(fd1);
		}
	}

out:
	syscall(319);
	shutdown(fd, SHUT_RDWR);
	close(fd);
	syscall(320);
}

void* racey_worker(void* data)
{
	int fd;
	int n = 0;
	char buf[256];
	int tid = *(int *)data;
	struct sockaddr_in myAddr;
        socklen_t mylen;

	char *buff= malloc(MAX_BUFF);
	if(!buff){
		printf("Impossible to malloc\n");
		exit(1);
	}
	//char buff[500];

	while (barrier == 0) {}

	for (;;) {
		/* Waiting for an availiable socket */
		for (;;) {
			syscall(319);
			pthread_mutex_lock(&client_fds_lock);
			syscall(320);
			while (have_fd == 0) {
				syscall(319);
				pthread_cond_wait(&cv, &client_fds_lock);
				syscall(320);
			}
			if ( client_idx>=0 && client_fds[client_idx] > 0) {
				fd = client_fds[client_idx];
				client_fds[client_idx] = -1;
				client_idx --;
				fault_count ++;
				if (client_idx == -1) {
					have_fd = 0;
				}
				pthread_mutex_unlock(&client_fds_lock);
				syscall(321, 1);
				break;
			}
			pthread_mutex_unlock(&client_fds_lock);
		}

	/*	if(fault_count==connection_before_error)
			syscall(318);

		do {
			syscall(319);
			n = read(fd, buf, sizeof(buf));
			//n = recv(fd, buf + 10, sizeof(buf), 0);
			syscall(320);
			sprintf(buf, "\n%d, %d", tid, fd);
			syscall(319);
			write(output_fd, buf, n);
			syscall(320);
		} while (n > 0);

		syscall(319);
		close(fd);
		syscall(320);*/
		//mylen= sizeof(struct sockaddr_in);
		//getpeername(fd, (struct sockaddr *) &myAddr, &mylen);
		//printf("thread %d handling connection port %d\n", tid, ntohs(myAddr.sin_port));
		connection(fd, buff);
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

	if (argc != 3)     /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <Server Port> <Connection_before_error>\n", argv[0]);
		exit(1);
	}

	server_port = atoi(argv[1]);  /* First arg:  local port */

	connection_before_error=  atoi(argv[2]);

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
		syscall(321, 20);
		syscall(319);
		if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr,
						&client_len)) < 0) {
			syscall(320);
			return 1;
		}
		syscall(320);
		while (1) {
			syscall(319);
			pthread_mutex_lock(&client_fds_lock);
			syscall(320);
			syscall(321, 3);
			if (client_idx < WORKER_NUM -1) {
				/* Feed the socket to workers */
				client_idx ++;
				have_fd = 1;
				client_fds[client_idx] = client_fd;
				syscall(319);
				pthread_cond_signal(&cv);
				syscall(320);
				pthread_mutex_unlock(&client_fds_lock);
				break;
			}
			pthread_mutex_unlock(&client_fds_lock);
		}
	}
}
