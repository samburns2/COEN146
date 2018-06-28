/*
 * Sam Burns
 * COEN 146L
 * F 2:15PM
 * 3/2/18
 *
 * lab5.c
 */

struct machine
{
	char name[50];
	char IP[50];
	int port;
};

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

pthread_mutex_t mutex;

void *receive_info();
void *link_state();
void printCostTable();
void printHostTable();
void printLeastCost();
void dij_alg(int costmatrix[100][100], int source);

int myID; 
int n = 4;
int my_sock;
int myPort;
int dataIn[3];
int dataOut[3];
int leastCost[100];
struct machine myMachines[100];
int costMatrix[100][100];

int sock, nBytes;
struct sockaddr_in serverAddr;
socklen_t addr_size;

int main(int argc, char *argv[])
{
	int i, j;

	if (argc != 5)
	{
		printf("Incorrect Parameters\n");
		printf("Usage: <current machine ID> <# of total machines> <cost table file> <hosts file>\n");
		return 1;
	}

	n = (int)atoi(argv[2]);
	myID = (short)atoi(argv[1]);
	
	
	FILE *costTable;
	costTable = fopen(argv[3], "r");
	if(costTable == NULL)
	{
		printf("Cost Table file could not be opened\n");
		return 1;
	}

	i = 0;
	while (!feof(costTable))
	{
			j = 0;
			while (j < n)
			{
				fscanf(costTable, "%d", &costMatrix[i][j]);
				j++;
			}
			i++;
	}
	fclose(costTable);

	printf("Initial Cost Table:\n");
	printCostTable();

	FILE *hostTable;
	hostTable = fopen(argv[4], "r");
	i = 0;
	while (!feof(hostTable) && i < n)
	{
		fscanf(hostTable, "%s%s%d", myMachines[i].name, myMachines[i].IP, &myMachines[i].port);
		if (myID == i)
			myPort = myMachines[i].port;
		i++;
	}
	fclose (hostTable);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((short)myPort);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset((char *)serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
	addr_size = sizeof(serverAddr);

	//create socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Socket Error\n");
		return 1;
	}
	
	if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		printf("Bind Error\n");
		return 1;
	}

	//start threads
	pthread_t receive;
	pthread_t linkState;
	pthread_mutex_init(&mutex, NULL);

	pthread_create(&receive, NULL, receive_info, NULL);
	pthread_create(&linkState, NULL, link_state, NULL);
	
	int changes = 0;
	while(changes < 2)
	{
		printf("Please enter your update to the cost table: ");
		printf("<neighbor> <new cost>\n");

		int neighbor;
		int newCost;
		scanf("%d%d", &neighbor, &newCost);

		pthread_mutex_lock(&mutex);

		costMatrix[myID][neighbor] = newCost;
		costMatrix[neighbor][myID] = newCost;

		printf("New Cost Table:\n");
		printCostTable();

		dataOut[0] = myID;
		dataOut[1] = neighbor;
		dataOut[2] = newCost;

		
		for (i = 0; i < n; i++)
		{
			if (i == myID)
				continue;
			else
			{
				serverAddr.sin_port = htons(myMachines[i].port);
				inet_pton(AF_INET, myMachines[i].IP, &serverAddr.sin_addr.s_addr);
	
				printf("Sending Update to %s:%d\n", myMachines[i].IP, myMachines[i].port);

				sendto(sock, dataOut, sizeof(dataOut), 0, (struct sockaddr *)&serverAddr, addr_size);
			}
		}

		pthread_mutex_unlock(&mutex);
		changes++;
		sleep(10);
	}
	sleep(30);
	fclose(costTable);
	fclose(hostTable);
	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
	return 0;
}

void printCostTable(int n, int costMatrix[][100])
{
	printf("\n");
	int i, j;
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			printf("%d\t", costMatrix[i][j], i, j);
		}
		printf("\n");
	}
}

void printHostTable(int n, struct machine myMachines[])
{
	printf("\n");
	int i;
	for (i = 0; i < n; i++)
	{
		printf("%s\t%s\t%d\n", myMachines[i].name, myMachines[i].IP, myMachines[i].port);
	}
}

void* receive_info()
{
	int theirID, neighborID, newCost;

	while (1)
	{		
		recvfrom(sock, &dataIn, sizeof(dataIn), 0, NULL, NULL);
		printf("Update received\n");
	
		theirID = dataIn[0];
		neighborID = dataIn[1];
		newCost = dataIn[2];

		pthread_mutex_lock(&mutex);
		
		costMatrix[theirID][neighborID] = newCost;
		costMatrix[neighborID][theirID] = newCost;

		printf("New Cost Table:\n");
		printCostTable(n, costMatrix);
		pthread_mutex_unlock(&mutex);
	}
}

void* link_state()
{
	while(1)
	{
		dij_alg(costMatrix, myID);
		printLeastCost();
		sleep(10 + (rand() % 11));
	}
}

void printLeastCost()
{
	printf("Least Cost Array:\n");

	int i;
	for (i = 0; i < n; i++)
		printf("%d ", leastCost[i]);
	printf("\n");
}

void dij_alg(int costmatrix[100][100], int source)
{
	int distance[4], visited[4] = {0}, start, min, m , d;

	int i;
	for (i = 0; i < n; i++)
		distance[i] = 10000;

	distance[source] = 0;

	start = source;
	visited[source] = 1;
	distance[start] = 0;

	int count = n - 1;

	while (count > 0)
	{
		min = 10000;
		m = 0;

		for (i = 0; i < n; i++)
		{
			d = distance[start] + costMatrix[start][i];
			if (d < distance[i] && visited[i] == 0)
			{
				min = distance[i];
				m = i;
			}
		}
		start = m;
		visited[start] = 1;
		count--;
	}

	for (i = 0; i < n; i++)
		leastCost[i] = distance[i];
}
