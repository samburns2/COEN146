/* Sam Burns
 * COEN 146L
 * F 2:15
 * 2/2/18
 *
 * client.c
 *
 * This program acts as the client side of a client/server application that transmits a text file
 * over the network using a UDP socket and copies it to a destination file on the other side.
 *
 * The program is a stop and wait type program. The client will not send the next packet until an
 * ACK for the previous packet is received
 *
 * To compensate for potential lost packets due to the use of UDP, the application uses ACKs
 *
 * This program takes in 4 arguments: <server port #> <server IP> <source file> <destination file>
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "../structures.h"

#define BUFF_SIZE 10

int main (int argc, char *argv[])
{
	int sock, portNum, nBytes;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	//check for correct parameters
	if (argc != 5)
	{
		printf("Incorrect parameters\n");
		printf("Usage: <port>, <Server IP>, <source file>, <destination file>\n");	
		return 1;
	}

	//configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
	addr_size = sizeof serverAddr;

	//Create UDP socket
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	
	int fileACK = 0;
	int filePacketCreated = 0;
	int seq = 0;

	PACKET filename;
	PACKET *pak;

	//loop to send filename in packet. file name must be 10 or less bytes
	while (fileACK == 0)
	{
		if (filePacketCreated == 0)
		{
			//initalize data
			strcpy(filename.data, argv[4]);

			//initalize length, checksum, seq/ack
			filename.header.length = strlen(filename.data) + 1;
			filename.header.seq_ack = seq;
			filename.header.checksum = 0;

			//calculate checksum
			filename.header.checksum = calcChecksum(&filename);

			filePacketCreated = 1;
		}
		
		//send filename packet
		sendto(sock, &filename, sizeof(filename), 0, (struct sockaddr *)&serverAddr, addr_size);

		//wait for ACK
		PACKET ACKorNAK;
		nBytes = recvfrom(sock, &ACKorNAK, sizeof(ACKorNAK), 0, NULL, NULL);

		if(ACKorNAK.header.seq_ack == 0)
		{
			fileACK = 1;
			seq = (seq == 0 ? 1 : 0);
			printf("Filename ACK received\n");
		}
		else
			printf("Filename NAK received. Resending filename\n");
	}

	//open source file and make sure it's valid
	FILE *src;
	src = fopen(argv[3], "r");
	if (src == NULL)
	{
		printf("The file %s could not be opened\n", argv[3]);
		return 1;
	}

	int pak0Created = 0;
	int pak1Created = 0;

	PACKET pak0;
	PACKET pak1;

	srand(time(NULL));

	//loop to go through file and transmit
	while(!feof(src))
	{
		//switch determines which sequence number a packet has
		switch(seq)
		{
			case 0:
				if (pak0Created == 0)
				{
					//read 10 bytes of data into packet
					nBytes = fread(pak0.data, sizeof(char), sizeof(pak0.data), src); 
					
					//intialize seq, length, and checksum
					pak0.header.seq_ack = seq;
					pak0.header.length = nBytes;
					pak0.header.checksum = 0;

					//calculate checksum
					pak0Created = 1;
				}

				//random number is generated to simulate dropped packet
				int randomNumber = rand() % 2;
				if (randomNumber == 0)
				{
					printf("FAKE ERROR\n");
					pak0.header.checksum = 0;
				}
				else
				{
					printf("NO FAKE ERROR\n");
					pak0.header.checksum = calcChecksum(&pak0);
				}
				//send packet to socket
				sendto(sock, &pak0, sizeof(pak0), 0, (struct sockaddr *)&serverAddr, addr_size);
				printf("Packet 0 sent\n");

				//wait for corresponding ACK
				PACKET ACK0;			
				nBytes = recvfrom(sock, &ACK0, sizeof(ACK0), 0, NULL, NULL);

				if(ACK0.header.seq_ack == seq) //Packet was acknowleged
				{
					
					printf("ACK0 Received\n");
					seq = 1;
					pak0Created = 0;
					break;
				}
				else
				{
					printf("NAK0 Received. Resending Packet\n");
				}

				break;
			case 1:
				if (pak1Created == 0)
				{
					//read 10 bytes of data into packet
					nBytes = fread(pak1.data, sizeof(char), sizeof(pak1.data), src); 
					
					//intialize seq, length, and checksum
					pak1.header.seq_ack = seq;
					pak1.header.length = nBytes;
					pak1.header.checksum = 0;

					//calculate checksum
					pak1Created = 1;
				}

				//random number to simulate dropped packets
				randomNumber = rand() % 2;
				if (randomNumber == 0)
				{
					printf("FAKE ERROR\n");
					pak1.header.checksum = 0;
				}
				else
				{
					printf("NO FAKE ERROR\n");
					pak1.header.checksum = calcChecksum(&pak1);
				}

				//send packet to socket
				sendto(sock, &pak1, sizeof(pak1), 0, (struct sockaddr *)&serverAddr, addr_size);

				printf("Packet 1 sent\n");

				//wait for corresponding ACK
				PACKET ACK1;			
				nBytes = recvfrom(sock, &ACK1, sizeof(ACK1), 0, NULL, NULL);

				if(ACK1.header.seq_ack == seq) //Packet was acknowleged
				{
					printf("ACK1 Received\n");
					seq = 0;
					pak1Created = 0;
					break;
				}
				else
				{
					printf("NAK1 Received. Resending Packet\n");
				}
		}
	}

	//send packet with length = 0 once file is finished being sent
	PACKET doneSending;
	doneSending.header.length = 0;

	sendto(sock, &doneSending, sizeof(doneSending), 0, (struct sockaddr *)&serverAddr, addr_size);

	fclose(src);

	return 0;
}


