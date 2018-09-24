Walker Schmidt
CSCI 4273
Programming Assignment #1

Two folders are found in the root directory of the project, a client and a server folder. In each of these these folders, a Makefile and the folder's respective C file is found.

To run this program, put the server folder on the server machine and the client folder on the client machine. Build the server and client code with the make command.


Run the server with the command:
./server <port>
where port is the port you want to bind to.

Run the client with the command:
./client <IP address> <port>
where IP address is the address of the machine running the server code and port is the port number that the server is bound to.


An interface will appear to the client. The client has a choice of 4 commands:

get <filename>
Gets the file from the server and copies it to the host's current directory

put <filename>
Puts the file from the client onto the server's current directory

delete <filename>
Deletes the file named filename on the server's current directory

ls
Lists all of the files in the server's current directory