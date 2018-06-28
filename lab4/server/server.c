/* Sam Burns
 * COEN 146L
 * F 2:15
 * 2/16/18
 *
 * server.c 
 *
 * This program acts as the server side of a client/server program that sends a text file
 * over the network, using UDP, and copies it to the destination file on the other side.
 *
 * This server.c program will send and ACK if the correct packet has been received by checking
 * the calculated checksum against the checksum received in the packet.
 *
 * There's a random part to this program where the server will only send the ACK
 * 80% of the time, to simulate error.
 *
 * This program takes in one argument: <port number>
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "../structures.h"

int main (int argc, char *argv[])
{
	int sock, nBytes;
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

	//check for correct arguments
	if (argc != 2)
	{
		printf("Incorrect Paramters\n");
		printf("Usage: <port number>\n");
		return 1;
	}

	//init
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((short) atoi(argv[1]));
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset ((char*)serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
	addr_size = sizeof(serverStorage);

	//create socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Socket Error\n");
		return 1;
	}

	//bind
	if (bind(sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0)
	{
		printf("Bind Error\n");
		return 1;
	}

	int fileOpened = 0;
	PACKET filename;
	FILE *dest;
	int origSum, checksum;

	//loop to continually check for packets
	while (1)
	{
		int randomNumber = rand() % 10;

		if (fileOpened == 0)
		{
			//receive packet with dest file name
			recvfrom(sock, &filename, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);


			//calculate checksum
			origSum = filename.header.checksum;
			filename.header.checksum = 0;

			checksum = calcChecksum(&filename);

			if (randomNumber > 1)
			{
				//if correct packet was received, send ACK
				if (checksum == origSum)
				{
					PACKET fileACK;
					fileACK.header.length = 0;
					fileACK.header.seq_ack = filename.header.seq_ack;

					sendto(sock, &fileACK, sizeof(fileACK), 0, (struct sockaddr *)&serverStorage, addr_size);
			
					printf("Filename ACK Sent\n");
				}
				else
				{
					PACKET fileNAK;
					fileNAK.header.seq_ack = (filename.header.seq_ack == 0 ? 1 : 0);
					fileNAK.header.length = 0;
					sendto(sock, &fileNAK, sizeof(fileNAK), 0, (struct sockaddr *)&serverStorage, addr_size);
			
					printf("Filename NAK Sent\n");
					continue;
				}
			}
			else
				continue;

			//open the file
			dest = fopen(filename.data, "w");

			if (dest == NULL)
			{
				printf("The file %s could not be opened\n", filename.data);
				return 1;
			}
			fileOpened = 1;
		}
	
		PACKET recvPacket;
		
		//wait for next packet
		recvfrom(sock, &recvPacket, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);
		
		//end waiting once last packet is received
		if (recvPacket.header.length == 0)
		{
			printf("Last packet received\n");
			fclose(dest);
			fileOpened = 0;
			continue;
		}

		//store received checksum calculate checksum of received packet
		origSum = recvPacket.header.checksum;
		recvPacket.header.checksum = 0;
		checksum = calcChecksum(&recvPacket);
		
		if (randomNumber > 1)
		{
			//if checksum was correct, send ACK, write data from packet to file
			if (checksum == origSum)
			{
				PACKET ACK;
				ACK.header.length = 0;
				ACK.header.seq_ack = recvPacket.header.seq_ack;

				sendto(sock, &ACK, sizeof(ACK), 0, (struct sockaddr *)&serverStorage, addr_size);
				printf("ACK sent\n");

				fwrite(recvPacket.data, sizeof(char), recvPacket.header.length, dest);

				continue;
			}
			else
			{
				PACKET NAK;
				NAK.header.length = 0;
				NAK.header.seq_ack = (recvPacket.header.seq_ack == 0 ? 1 : 0);

				sendto(sock, &NAK, sizeof(NAK), 0, (struct sockaddr *)&serverStorage, addr_size);

				printf("NAK sent\n");
				continue;
			}
		}
		else
			continue;
	}
}


