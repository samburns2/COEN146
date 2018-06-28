/*
 * Sam Burns
 * COEN 146L
 * F 2:15
 * 1/19/18
 *
 * client.c:
 * This program implements the client side of a network application 
 * that takes a text file, sends it over the network, and copies it 
 * to the given destination file on the other side. The program uses
 * TCP in order to establish a connection with the server.
 *
 * The program takes in 4 arguments: <server port>, <server IP>, <source file>, <destination> file.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>

int main (int argc, char* argv[])
{
	int i;
	int sockfd = 0, n = 0;
	char buff [10];
	struct sockaddr_in serv_addr;

	if (argc != 5)
	{
		printf("\nIncorrect Parameters");
		printf("\nUsage: <port>, <Server IP Address>, <source file>, <destination file>");
		return 1;
	}

	//set up
	memset (buff, '0', sizeof (buff));
	memset(&serv_addr, '0', sizeof(serv_addr));

	//open socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error: Could not create socket\n");
	}

	//set address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
	{
		printf("inet_pton error occurred\n");
		return 1;
	}

	//connect
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Error: Connect Failed\n");
		return 1;
	}

	//send destination filename
	write(sockfd, argv[4], strlen (argv[4]) + 1);

	//open source file and make sure it's valid
	FILE *src;
	src = fopen(argv[3], "r");
	if (src == NULL)
	{
		printf("\nThe file %s, could not be opened\n", argv[3]);
		return 1;
	}

	//go through entire source file and write to socket
	while (!feof(src))
	{
		size_t bytesRead = fread(buff, sizeof(char), sizeof(buff), src);
		write(sockfd, buff, bytesRead);
	}

	fclose(src);
	close(sockfd);
}
