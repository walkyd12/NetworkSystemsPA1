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
#include <fcntl.h>

#define MAXBUFSIZE 100
#define DEBUG 1

char *END_FLAG = "#End of File#";
char* SUCC_FLAG = "ok";
char* FILE_ERR_FLAG = "file not found.";
char* RESEND_FLAG = "please resend";
char* DONT_RESEND_FLAG = "dont resend";

int PutFile(int sock, struct sockaddr_in remote,char* filename);
int GetFile(int sock, struct sockaddr_in remote, char* filename, unsigned int remote_length);

unsigned checksum(void *buffer, size_t len, unsigned int seed) {
      unsigned char *buf = (unsigned char *)buffer;
      size_t i;

      for (i = 0; i < len; ++i)
            seed += (unsigned int)(*buf++);
      return seed;
}

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

	// fcntl(sock, F_SETFL, O_NONBLOCK);

	char command[MAXBUFSIZE];
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

		memset(command, 0, MAXBUFSIZE);
		// Get user input and remove new line characters
		fgets(command, MAXBUFSIZE, stdin);
		if ((strlen(command) > 0) && (command[strlen(command) - 1] == '\n'))
			command[strlen(command) - 1] = '\0';

		int ret;

		// Check to make sure user entered a valid command
		if( (0 != memcmp("get", command, 3)) && (0 != memcmp("put", command, 3)) && (0 != memcmp("ls", command, 2)) &&
			 (0 != memcmp("delete", command, 6)) && (0 != memcmp("exit", command, 4)) ) {
			printf("Please enter a valid command.\n");
			continue;
		}

		// Send command to server
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
	n = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	// File not found error check
	if (!(strcmp(filename, FILE_ERR_FLAG))) {
        return 0;
    }
    // Open a new file with the name file name with read, write and create flags set
	fd = open(filename, O_RDWR | O_CREAT, 0666);

	// Store file contents in filename buffer
	while ((n = recvfrom(sock, filename, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length))) {
	    if (!(memcmp(filename, END_FLAG, sizeof(&END_FLAG))) ) {
	        break;
	    }
	    if(!(memcmp(filename, FILE_ERR_FLAG, sizeof(&FILE_ERR_FLAG)))) {
	    	if(remove(filename) != 0) {
				sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
				return 0;
			}
	    }
	    // Write the contents of the buffer from the file stream and write it to the file we created
	    write(fd, filename, n);
	}
	close(fd);

	return 1;
}
int PutFile(int sock, struct sockaddr_in remote, char* filename) {
	int n, fd, nbytes;
	unsigned int remote_length;

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
    
	int offset = 0;
	int fileSize;
	while(1) {
		// The file was opened succesfully, read from the file into the filename buffer
		while ((n = read(fd, (filename + offset), MAXBUFSIZE)) > 0) {
			// send the buffer to the server
			if(offset == 0)
				fileSize = n;
		    sendto(sock, filename, n, 0, (struct sockaddr *)&remote, sizeof(remote));
		}
		offset += MAXBUFSIZE;
		if(offset > fileSize)
			break;
	}
	// Send the end of file signal to the server
	sendto(sock, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
    close(fd);

	return 1;
}




