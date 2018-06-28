/*
 * Sam Burns
 * COEN 146L
 * F 2:15PM
 * 3/2/18
 *
 * lab5.c
 *
 * This program is an imiplementation of code that acts as routers running a link-state algorithm. The program has 3 threads:
 *
 * Thread 1:Loops forever. Receives messages from other nodes and updates cost table
 * Thread 2:Reads new change from user every 10 seconds. Updates cost table and sends updates to other nodes.
 * Thread 3: Implementation of djikstra's algorithm for the least cost array
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
	
	//parse cost table
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

	//parse host table
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

		//send update to other nodes
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

//function to print out the cost table
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

//function to print out the host table
void printHostTable(int n, struct machine myMachines[])
{
	printf("\n");
	int i;
	for (i = 0; i < n; i++)
	{
		printf("%s\t%s\t%d\n", myMachines[i].name, myMachines[i].IP, myMachines[i].port);
	}
}

//thread 1
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

//thread 3
void* link_state()
{
	while(1)
	{
		dij_alg(costMatrix, myID);
		printLeastCost();
		sleep(10 + (rand() % 11));
	}
}

//function to print out the least cost array
void printLeastCost()
{
	printf("Least Cost Array:\n");

	int i;
	for (i = 0; i < n; i++)
		printf("%d ", leastCost[i]);
	printf("\n");
}


//implementation of djikstra's algorithm.
void dij_alg(int costmatrix[100][100], int source)
{
    // init variables
    int distance[4], visited[4] = {0}, min, m, d, start, i;
    
    for(i = 0; i < n; i++)
        distance[i] = 10000; // init all best paths to some large number
    
    distance[source] = 0; // init node of interest path value
    
    start = source; // init start to node of interest
    visited[source] = 1; // label node as visited
    distance[start] = 0; // init distance of node
    
    int count = n - 1; // init count for loop
    
    while(count > 0)
    {
        min = 10000; // init min
        m = 0; // init m
        for(i = 0; i < n; i++)
        {
            d = distance[start] + costMatrix[start][i]; // as long as nodes calculate cost and dist
            if(d < distance[i] && visited[i] == 0)
                distance[i] = d; // if not visited and d is less that current distance, assign
            
            if(min > distance[i] && visited[i] == 0)
            {
                min = distance[i]; // if this new value is less than min, assign to min
                m = i; // assign index of node to m
            }
        }
        start = m; // init start to m
        visited[start] = 1; // label this node as visited
        count--; // decrement counter
    }
    
    for( i = 0; i < n; ++i){
        leastCost[i] = distance[i]; // store all distances in global least cost variable
    }
}

