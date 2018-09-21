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

#define MAXBUFSIZE 1000000
#define cipherKey 'S'

char *END_FLAG = "ENDOFFILEENDOFFILEENDOFFILE";
char* SUCC_FLAG = "ok";
char* FILE_ERR_FLAG = "File not found.";

int PutFile(int sock, struct sockaddr_in remote,char* filename);
int GetFile(int sock, struct sockaddr_in remote, char* filename, unsigned int remote_length);

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

		int ret;

		nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *) &remote, sizeof(remote));
		
		if(0 == memcmp("put", command, 3)) {
			ret = PutFile(sock, remote, command+4);
			if(ret == 0)
				continue;
		}
		if(0 == memcmp("get", command, 3)) {
			ret = GetFile(sock, remote, command+4, remote_length);
			if(ret == 0)
				continue;
		}
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
int GetFile(int sock, struct sockaddr_in remote, char* filename, unsigned int remote_length) {
	int fd, n, nbytes;
	
	// Store server acknowledgement in buffer
	char buffer[100];
	// Wait for server to acknowledge if file is found
	n = recvfrom(sock, buffer, MAXBUFSIZE-4, 0, (struct sockaddr *)&remote, &remote_length);
	// File not found error check
	if (!(strcmp(filename, FILE_ERR_FLAG))) {
        return 0;
    }
    // Open a new file with the name file name with read, write and create flags set
	fd = open(filename, O_RDWR | O_CREAT, 0666);

	// Store file contents in filename buffer
    while ((n = recvfrom(sock, filename, MAXBUFSIZE-4, 0, (struct sockaddr *)&remote, &remote_length))) {
        if (!(strcmp(filename, END_FLAG)) || !(strcmp(filename, FILE_ERR_FLAG))) {
            break;
        }
        // Write the contents of the buffer from the file stream and write it to the file we created
        write(fd, filename, n);
    }
    close(fd);

	return 1;
}
int PutFile(int sock, struct sockaddr_in remote, char* filename) {
	int n, fd, nbytes;

	// Attempt to open the file with read only priveledges
    fd = open(filename, 'r');

    // if the file descriptor is less than 0, the file was not opened successfully
    if (fd < 0) {
    	// Print client error message
    	printf("\nFile open failed! File: %s\n", filename);
    	// Send file error signal to server
		sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
        return 0;
	}
    
    // The file was opened succesfully, read from the file into the filename buffer
    while ((n = read(fd, filename, MAXBUFSIZE-4)) > 0) {
    	// send the buffer to the server
        sendto(sock, filename, n, 0, (struct sockaddr *)&remote, sizeof(remote));
    }
    // Send the end of file signal to the server
    sendto(sock, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));

	return 1;
}




