#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
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
#include <dirent.h>
#include <fcntl.h>

#include "sha2.h"
/* You will have to modify the program below */

#define MAXBUFSIZE 		100
#define DEBUG 			1
#define MAX_RESEND		1

char* END_FLAG = "#End of file.#";
char* SUCC_FLAG = "ok";
char* FAIL_FLAG = "not ok";
char* FILE_ERR_FLAG = "file not found.";
char* FILE_SUCC_FLAG = "file found.";
char* RESEND_FLAG = "please resend";
char* DONT_RESEND_FLAG = "dont resend";

int GetFile(int sock, struct sockaddr_in remote,char* filename);
int PutFile(int sock, struct sockaddr_in remote,char* filename, unsigned int remote_length);
int DeleteFile(int sock, struct sockaddr_in remote,char* filename);
int List(int sock, struct sockaddr_in remote);

static void hash_to_string(char string[65], const uint8_t hash[32])
{
	size_t i;
	for (i = 0; i < 32; i++) {
		string += sprintf(string, "%02x", hash[i]);
	}
}

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

		if(ret == 0)
			printf("File transfer error.\n");
	}

	close(sock);
}

int GetFile(int sock, struct sockaddr_in remote, char* filename) {
	int n, fd, nbytes;
	unsigned int remote_length;

	// Attempt to open file named filename, read only
    fd = open(filename, 'r');

    struct timeval watch_stop, watch_start;
	gettimeofday(&watch_start, NULL);

    // File does not exist
    if (fd < 0) {
    	printf("\nFile open failed! File: %s\n", filename);
		// Send file not found error to client
		sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
        return 0;
	}

    // Found a file, send file found message to client
    sendto(sock, FILE_SUCC_FLAG, strlen(FILE_SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
    
	uint8_t hash[32];
	char hash_string[65];
	char returnBuffer[1];
    // Read the contents of the file and store it in the filename buffer
    while ((n = read(fd, filename, MAXBUFSIZE)) > 0) {
        printf("Sending n bytes: %d\n", n);

		memset(hash, 32, 0);
		memset(hash_string, 65, 0);
		memset(returnBuffer, 100, 0);

        // Send this data to the client
        sendto(sock, filename, n, 0, (struct sockaddr *)&remote, sizeof(remote));


	    calc_sha_256(hash, filename, n);
		hash_to_string(hash_string, hash);
		printf("Hash: %s\n", hash_string);

		sendto(sock, hash_string, 65, 0, (struct sockaddr *)&remote, sizeof(remote));

		recvfrom(sock, returnBuffer, 1, 0, (struct sockaddr *)&remote, &remote_length);

		if(returnBuffer[0] == 'y') {
			int shouldContinue = 1;
			while(shouldContinue != 0) {
				printf("Resending n bytes: %d\n", n);

				// send the buffer to the server
	    		sendto(sock, filename, n, 0, (struct sockaddr *)&remote, sizeof(remote));
	    		sendto(sock, hash_string, 65, 0, (struct sockaddr *)&remote, sizeof(remote));

	    		recvfrom(sock, returnBuffer, 1, 0, (struct sockaddr *)&remote, &remote_length);

	    		if(returnBuffer[0] == 'n')
	    			shouldContinue = 0;
			}
		}
    }
    // Send the end of file flag to the client
    sendto(sock, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
    // Send success message back to client
    sendto(sock, SUCC_FLAG, strlen(SUCC_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));

    close(fd);

    gettimeofday(&watch_stop, NULL);
#ifdef DEBUG
	printf("Get took %d\n", watch_stop.tv_usec - watch_start.tv_usec);
#endif
	return 1;
}
int PutFile(int sock, struct sockaddr_in remote, char* filename, unsigned int remote_length) {
	int fd, n, nbytes;

	// Open a new file with the name file name with read, write and create flags set
	fd = open(filename, O_RDWR | O_CREAT, 0666);

	struct timeval watch_stop, watch_start;
	gettimeofday(&watch_start, NULL);

	uint8_t hash[32];
	char hashString[65];
	char oldHashString[65];
	char yesRet[1] = { "y" };
	char noRet[1] = { "n" };
	// Store file contents in filename buffer
	while ((n = recvfrom(sock, filename, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length))) {
	    printf("Recieved n bytes: %d\n", n);
	    // If we get the signal of the end of the file or if there is an error, stop waiting for data
	    if ( !(memcmp(filename, END_FLAG, sizeof(&END_FLAG))) ) {
	        break;
	    }
	    if( !(memcmp(filename, FILE_ERR_FLAG, sizeof(&FILE_ERR_FLAG))) ) {
	    	if(remove(filename) != 0) {
				sendto(sock, FILE_ERR_FLAG, strlen(FILE_ERR_FLAG), 0, (struct sockaddr *)&remote, sizeof(remote));
				return 0;
			}
	    }

	    recvfrom(sock, oldHashString, 65, 0, (struct sockaddr *)&remote, &remote_length);
	
	    calc_sha_256(hash, filename, n);
		hash_to_string(hashString, hash);

		// printf("Hash compare: %s | %s\n", hashString, oldHashString);

		if(0 == memcmp(hashString, oldHashString, 65)) {
			sendto(sock, noRet, 1, 0, (struct sockaddr *)&remote, sizeof(remote));
			printf("Hash Match!\n");
		} else {
			sendto(sock, yesRet, 1, 0, (struct sockaddr *)&remote, sizeof(remote));
			printf("HASHES DONT MATCH\n");
		}
	    // write data from the filename buffer into the new file
	    write(fd, filename, n);
	}
	
	close(fd);

    gettimeofday(&watch_stop, NULL);
#ifdef DEBUG
	printf("Put took %d\n", watch_stop.tv_usec - watch_start.tv_usec);
#endif

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
	char buffer[MAXBUFSIZE] = {"\n\n"};
	DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] != '.') {
            	strcat(buffer, dir->d_name);
            	strcat(buffer, "\n");
        	}
        }
        closedir(d);
        buffer[strlen(buffer) - 1] = '\0'; 
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&remote, sizeof(remote));
    }
	return 1;
}
