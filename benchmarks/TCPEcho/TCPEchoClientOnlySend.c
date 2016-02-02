#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>

#define RCVBUFSIZE 100   /* Size of receive buffer */

int main(int argc, char *argv[])
{
    int sock;                        
    struct sockaddr_in echoServAddr; 
    unsigned short echoServPort;     
    char *servIP;                    
    char *read_from_name;               
    char echoBuffer[RCVBUFSIZE];     
    unsigned int echoStringLen;     
    int bytesRcvd, totalBytesRcvd;   
    int read_from, file_size;
    struct stat st;

    if ((argc != 4))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <File> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    read_from_name = argv[2];       

    echoServPort = atoi(argv[3]);
 	
    read_from= open(read_from_name, O_RDONLY);
    if(read_from==-1){
	printf("impossible to open %s\n", read_from_name);
        exit(-1);	
    }

    stat(read_from_name, &st);
    file_size = st.st_size;

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printf("socket() failed\n");
        exit(-1);
    }

    printf("socket created id %d\n",sock);

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(echoServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
        printf("connect failed\n");
        exit(-1);
    }

    printf("socket connected\n");

    if(send(sock, &file_size, sizeof(file_size), 0) != sizeof(file_size)){
        printf("send() sent a different number of bytes than expected for file size\n");
        exit(-1);
    }


    do{
    	echoStringLen = read(read_from, echoBuffer, RCVBUFSIZE);
    	if(echoStringLen>0){
    		/* Send the string to the server */
    		if (send(sock, echoBuffer, echoStringLen, 0) != echoStringLen){
        		printf("send() sent a different number of bytes than expected\n");
			exit(-1);
		}
	}
    }while(echoStringLen>0);

    printf("file sent\n");

    /*totalBytesRcvd = 0;
    printf("Received: ");   

    while (totalBytesRcvd < echoStringLen)
    {
        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        totalBytesRcvd += bytesRcvd;   
        echoBuffer[bytesRcvd] = '\0';  
        printf("%s", echoBuffer);      
    }*/
    recv(sock, echoBuffer, RCVBUFSIZE, 0);
    
    //printf("received %s\n", echoBuffer);
    
    close(read_from);
    close(sock);
    exit(0);
}

