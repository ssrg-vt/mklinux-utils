#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define __NR_ft_crash_kernel 318
#define RCVBUFSIZE 200

FILE *output;

void HandleTCPClient(int clntSocket)
{
    char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;
    int file_size;
    int bytes_recv=0;
    int write_to;

    fprintf(output,"starting\n");   
    fflush(output);
    
    // Receive size of file from client
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0){
        fprintf(output,"recv() failed\n");
        exit(0);
    }

    file_size= *((int*)echoBuffer);
    fprintf(output,"size file: %i\n", file_size);
    fflush(output);
	
    write_to= open("received_from_client.txt", O_WRONLY | O_CREAT, 0777);
    if(write_to==-1){
        fprintf(stderr, "Impossible to open file\n");
        exit(1);
    }
    

    do{

		if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0){
        		fprintf(output,"recv() failed\n");
        		exit(0);
    		}

		if(write(write_to, echoBuffer, recvMsgSize) != recvMsgSize){
			fprintf(output,"write() failed\n");
                        exit(0);

		}

		bytes_recv+= recvMsgSize;

		if( bytes_recv >= file_size)
			break;
 
    }while (recvMsgSize > 0);
        
    close(write_to);

    sprintf(echoBuffer,"%s","done");
    if (send(clntSocket, echoBuffer, sizeof("done"), 0) < 0){
    	fprintf(output,"send() failed\n");  	
	exit(0);
    }

    fprintf(output,"ending\n");
    fflush(output);

    close(clntSocket);    /* Close client socket */
}   

int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    struct timeval time;

    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    output= fopen("server_debug.txt", "w+");
    if(output==NULL){
        fprintf(stderr, "Impossible to open file\n");
        exit(1);
    }
    
    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fprintf(output,"socket() failed\n");
	exit(0);
    }

    fprintf(output,"socket created id %d\n",servSock);      

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
	fprintf(output,"bind() failed\n");
        exit(0);
    }

    fprintf(output,"socket binded\n");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0){
	fprintf(output,"listen() failed\n");
        exit(0);
    }   

    fprintf(output,"listening on socket\n");

    fflush(output);

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0){
	        fprintf(output,"accept() failed\n");
        	exit(0);
    	}   

        /* clntSock is connected to a client! */

        fprintf(output,"Handling client %s id %d\n", inet_ntoa(echoClntAddr.sin_addr), clntSock);
	
	fflush(output);
        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}

