#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

FILE *output;

void HandleTCPClient(int clntSocket)
{
	int recvMsgSize;           
	unsigned short port;

	/* Receive message from client */
	if ((recvMsgSize = recv(clntSocket, &port, sizeof(port), 0)) < 0){
		fprintf(output,"recv() failed\n");
		exit(0);
	}

	fprintf(output, "%hu\n", port);
	fflush(output);
	close(clntSocket);    /* Close client socket */
}

int main(int argc, char *argv[])
{
	int servSock;                    /* Socket descriptor for server */
	int clntSock;                    /* Socket descriptor for client */
	struct sockaddr_in servAddr; /* Local address */
	struct sockaddr_in clntAddr; /* Client address */
	unsigned short servPort;     /* Server port */
	unsigned int clntLen;            /* Length of client address data structure */
	int max_pending,i;

	if (argc != 3)     /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <Server Port> <connections before crash>\n", argv[0]);
		exit(1);
	}

	output= fopen("./ramcache/server_connections.txt", "w+");
	if(output==NULL){
		fprintf(stderr, "Impossible to open file\n");
		exit(1);
	}

	servPort = atoi(argv[1]);  /* First arg:  local port */
	max_pending= atoi(argv[2]);
	/* Create socket for incoming connections */
	if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		fprintf(output, "socket failed\n");
		exit(0);
	}

	/* Construct local address structure */
	memset(&servAddr, 0, sizeof(servAddr));   /* Zero out structure */
	servAddr.sin_family = AF_INET;                /* Internet address family */
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	servAddr.sin_port = htons(servPort);      /* Local port */

	/* Bind to the local address */
	if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
		fprintf(output, "bind failed\n");
		exit(0);
	}

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, max_pending*2) < 0){
		fprintf(output,"listen() failed\n");
		exit(0);
	}   

	fflush(output);

	 /* Set the size of the in-out parameter */
        clntLen = sizeof(clntAddr);

	for (i=0;;i++) /* Run forever */
	{
		if(i==max_pending){
			syscall(318);
		}

		/* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, 
						&clntLen)) < 0){
			fprintf(output,"accept() failed\n");
			exit(0);
		}   

		/* clntSock is connected to a client! */

		HandleTCPClient(clntSock);
	}
	/* NOT REACHED */
}

