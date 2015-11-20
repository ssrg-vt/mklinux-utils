#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */

int main(int argc, char *argv[])
{
	int sock;   
	struct sockaddr_in servAddr; 
	struct sockaddr_in myAddr;
	char *servIP;
	unsigned short servPort; 
	unsigned short myPort;
	int i, connections;
	socklen_t len;

	if ((argc != 4))    /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage: %s <Server IP> <Server Port> <Connections>\n",
				argv[0]);
		exit(1);
	}

	servIP = argv[1];             
	servPort = atoi(argv[2]); 
	connections= atoi(argv[3]);


	for (i=0; i<connections; i++){
		 /* Create a reliable, stream socket using TCP */
		if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			printf("socket() failed\n");
			exit(-1);
		}

		/* Construct the server address structure */
		memset(&servAddr, 0, sizeof(servAddr));     /* Zero out structure */
		servAddr.sin_family      = AF_INET;             /* Internet address family */
		servAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
		servAddr.sin_port        = htons(servPort); /* Server port */


		/* Establish the connection to the echo server */
		if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
			printf("connect failed\n");
			exit(-1);
		}

		len= sizeof(myAddr);
		if(getsockname(sock, (struct sockaddr *) &myAddr, &len) == 0){
    			myPort = ntohs(myAddr.sin_port);
		}
		else{
			printf("getsockname failed\n");
                        exit(-1);

		}
		
		printf("sending %hu\n", myPort);

		if (send(sock, &myPort, sizeof(myPort), 0) != sizeof(myPort)){
			printf("send() sent a different number of bytes than expected\n");
			exit(-1);
		}

		close(sock);

	}
	
	exit(0);
}

