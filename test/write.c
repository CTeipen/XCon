#include <fcntl.h> 
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 3

int main(int argc, char *argv[]) {

	int fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		/* An error occurred. Print an error message and bail.  */ 
		perror ("open"); 

		return 1; 
	} 

	int end = 0;
	while(!end) {

		char* buffer = calloc(SIZE, sizeof(char) * SIZE);
		char* stdBuffer = calloc(SIZE, sizeof(char) * 2);

		buffer[0] = 0x01;
		buffer[1] = 0x03;

		read(0, stdBuffer, 3);

		int number = atoi(stdBuffer);
		if(number == -1) {
			end = 1;
		} else {
			buffer[2] = atoi(stdBuffer);
			//buffer[3] = 0x01;
			//buffer[4] = 0x00;
			//buffer[5] = 0x00;

			int size = write(fd, buffer, SIZE);

			free(buffer);
			free(stdBuffer);

			printf("Size written: %d\n", size);
		
		}
		
	}

	close(fd);
	printf("Close..\n");

	return 0;

}
