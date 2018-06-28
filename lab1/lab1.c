/* Sam Burns
 * 1/12/18
 * COEN 146L
 * Friday 2:15
 */

#include <stdio.h>

#define BUFFER_SIZE 10

int main (int argc, char *argv[])
{
	FILE *src;
	FILE *dest;

	char buffer[BUFFER_SIZE];

	//if correct number of parameters were entered
	if (argc == 3)
	{
		//open source file and make sure it's valid
		src = fopen(argv[1], "r");
		if (src == NULL)
		{
			printf("\nThe file %s could not be opened\n", argv[1]);
			return 1;
		}

		//open destination file and make sure it's valid
		dest = fopen(argv[2], "w");
		if (dest == NULL)
		{
			printf("\nThe file %s could not be opened\n", argv[2]);
			return 1;
		}

		//go through the source file, read to buffer, and write from buffer to destination file
		while (!feof(src))
		{
			size_t bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE, src);
			fwrite(buffer, sizeof(char), bytesRead, dest);
		}

		fclose(src);
		fclose(dest);

	}
	//if incorrect number of parameters were entered
	else
	{
		printf("\nInvalid Parameters. Please enter 1 source file and 1 destination file as arguments.\n");
		return 1;
	}
}
