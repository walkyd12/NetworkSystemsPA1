#include <sys/types.h>
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
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 1000000
#define cipherKey 'S'

char* END_FLAG = "ENDOFFILEENDOFFILEENDOFFILE";
char* SUCC_FLAG = "ok";
char* FILE_ERR_FLAG = "File not found.";
char* FILE_SUCC_FLAG = "File found.";

int GetFile(int sock, struct sockaddr_in remote,char* filename);
int PutFile(int sock, struct sockaddr_in remote,char* filename, unsigned int remote_length);
int DeleteFile(int sock, struct sockaddr_in remote,char* filename);
int List(int sock, struct sockaddr_in remote);

int main (int argc, char * argv[] )
{
	int sock;							//This will be our socket
	struct sockaddr_in sin, remote;		//"Internet socket address structure"
	unsigned int remote_length;			//length of the sockaddr_in structure
	int nbytes;							//number of bytes we receive in our message
	char buffer[MAXBUFSIZE];			//a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0) ) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	while(1) {
		//waits for an incoming message
		bzero(buffer, sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);

		printf("The client says %s\n", buffer);

		int ret;
		char output[256];
		if(0 == memcmp("exit", buffer, 4))
			break;
		if(0 == memcmp("get", buffer, 3))
			ret = GetFile(sock, remote, buffer+4);
		if(0 == memcmp("put", buffer, 3))
			ret = PutFile(sock, remote, buffer+4, remote_length);
		if(0 == memcmp("delete", buffer, 6))
			ret = DeleteFile(sock, remote, buffer+7);
		if(0 == memcmp("ls", buffer, 2))
			ret = List(sock, remote);
	}

	close(sock);
}

int GetFile(int sock, struct sockaddr_in remote, char* filename) {
	int n, fd, nbytes;

	// Attempt to open file named filename, read only
    fd = open(filename, 'r');

    // File does not exist
    if (fd < 0) {
    	printf("\nFile open failed! File: %s\n", filename);
		// Send file not found error to client
		sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
        return 0;
	}
    // Found a file, send file found message to client
    sendto(sock, FILE_SUCC_FLAG, strlen(FILE_SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
    
    // Read the contents of the file and store it in the filename buffer
    while ((n = read(fd, filename, MAXBUFSIZE-4)) > 0) {
        // Send this data to the client
        sendto(sock, filename, n, 0, (struct sockaddr *)&remote, sizeof(remote));
    }
    // Send the end of file flag to the client
    sendto(sock, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
    // Send success message back to client
    sendto(sock, SUCC_FLAG, strlen(SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
	return 1;
}
int PutFile(int sock, struct sockaddr_in remote, char* filename, unsigned int remote_length) {
	int fd, n, nbytes;
	// Open a new file with the name file name with read, write and create flags set
	fd = open(filename, O_RDWR | O_CREAT, 0666);

	// Store file contents in filename buffer
    while ((n = recvfrom(sock, filename, MAXBUFSIZE-4, 0, (struct sockaddr *)&remote, &remote_length))) {
        // If we get the signal of the end of the file or if there is an error, stop waiting for data
        if (!(strcmp(filename, END_FLAG)) || !(strcmp(filename, FILE_ERR_FLAG))) {
            break;
        }
        // write data from the filename buffer into the new file
        write(fd, filename, n);
    }
    close(fd);
    // Send success signal back to client
	sendto(sock, SUCC_FLAG, strlen(SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));

	return 1;
}
int DeleteFile(int sock, struct sockaddr_in remote, char* filename) {
	// If remove filename is not 0, there is no such file so send error back
	if(remove(filename) != 0) {
		sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
		return 0;
	}
	// Otherwise, remove(filename) is 0 meaning the file was successfully removed, so send success signal
	sendto(sock, SUCC_FLAG, strlen(SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
	return 1;
}
int List(int sock, struct sockaddr_in remote) {
	return 1;
}
