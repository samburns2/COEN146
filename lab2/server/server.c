/*
 * Sam Burns
 * COEN 146L
 * F 2:15
 * 1/19/18
 *
 * server.c
 * This program acts as the server side of an application that 
 * takes in a file, sends it over the network, and copies that 
 * file to the given destination file on the other side.
 *
 * The progam takes in one argument: <port number>
 * The port number is used in setting up the server, and must be known by the client, along with the
 * server's IP Address in order to connect.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>


int main (int argc, char *argv[])
{
	int n;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	char buff[5];

	if (argc != 2)
	{
		printf("\nIncorrect parameters. Please enter the port number\n");
		return 1;
	}

	//set up
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(buff, '0', sizeof(buff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	//create the socket, bind, and listen
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(listenfd, 10);

	//accept and interact with client
	while (1)
	{
		connfd = accept (listenfd, (struct sockaddr*)NULL, NULL);
		char filename[1025];
		FILE *dest;

		if (read(connfd, filename, sizeof(filename)) > 0)
			dest = fopen(filename, "w");

		else
		{
			printf("Destination file could not be opened\n");
			return 1;
		}
	
		while ((n = read(connfd, buff, sizeof(buff))) > 0)
		{
			fwrite(buff, sizeof(char), n, dest);
		}

		fclose (dest);
		close (connfd);
	}
}
