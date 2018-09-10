#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#define MAXBUFSIZE 100

/* You will have to modify the program below */

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote;              //"Internet socket address structure"
	unsigned int remote_length = sizeof(remote);			//length of the sockaddr_in structure

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	while(1) {
		printf("\n");
		printf("-----------------------------------\n");
		printf("Type one of the following commands:\n");
		printf("-----------------------------------\n");
		printf("get [file_name]\n");
		printf("put [file_name]\n");
		printf("delete [file_name]\n");
		printf("ls\n\n");
		printf("exit\n");
		printf("-----------------------------------\n");

		char command[MAXBUFSIZE];
		// Get user input and remove new line characters
		fgets(command, MAXBUFSIZE, stdin);
		if ((strlen(command) > 0) && (command[strlen(command) - 1] == '\n'))
			command[strlen(command) - 1] = '\0';

		nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *) &remote, sizeof(remote));

		if(0 == memcmp("exit", command, 4))
			break;

		// Blocks till bytes are received
		struct sockaddr_in from_addr;
		int addr_length = sizeof(struct sockaddr);
		bzero(buffer, sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *) &remote, &remote_length);

		printf("Server says %s\n", buffer);
	}

	close(sock);

}

