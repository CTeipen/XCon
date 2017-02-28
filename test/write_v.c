#include <fcntl.h> 
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 8

int main(int argc, char *argv[]) {

	int fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		perror ("open"); 
		return -1; 
	} 

	int end = 0;
	char* buffer = calloc(SIZE, sizeof(char) * SIZE);
	char* stdBuffer = calloc(SIZE, sizeof(char) * 4);
	while(!end) {

		bzero(stdBuffer, 4);

		buffer[0] = 0x00;
		buffer[1] = 0x08;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;
		buffer[5] = 0x00;
		buffer[6] = 0x00;
		buffer[7] = 0x00;

		printf("Links (0) oder rechts (1) oder abbrechen (-1)?\n");
		read(0, stdBuffer, 2);
		fflush(0);

		int number = atoi(stdBuffer);
		if(number == -1) {
			end = 1;
		} else {
			
			printf("Staerke?\n");
			
			read(0, stdBuffer, 4);
			fflush(0);
			if(number == 0) {
				buffer[3] = atoi(stdBuffer);
			} else {
				buffer[4] = atoi(stdBuffer);
			}

			int size = write(fd, buffer, SIZE);

			printf("Size written: %d\n", size);
		
		}
		
	}

	free(buffer);
	free(stdBuffer);

	close(fd);
	printf("Close..\n");

	return 0;

}
