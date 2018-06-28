/* Sam Burns
 * COEN 146L
 * F 2:15
 * 2/2/18
 *
 * structures.h
 *
 * This .h file contains two structs (HEADER & PACKET) that are used in the server.c and client.c
 * programs
 *
 * It also contains the function used to calculate the checksum for the packets in the client/server
 * programs
 */

typedef struct{
	int seq_ack;
	int length;
	int checksum;
} HEADER;

typedef struct{
	HEADER header;
	char data[10];
} PACKET;

int calcChecksum (void *pak)
{
	unsigned char *p;
	p = pak;
	int i;
	int tempSum = 0;

	for (i = 0; i < sizeof(PACKET); i++)
		tempSum ^= *p++;

	return tempSum;
}
