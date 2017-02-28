#include <fcntl.h> 
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 3

int main(int argc, char *argv[]) {

	int fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		perror ("open"); 
		return 1; 
	} 

	int end = 0;
	char* buffer = calloc(SIZE, sizeof(char) * SIZE);
	char* stdBuffer = calloc(SIZE, sizeof(char) * 3);
	while(!end) {

		bzero(stdBuffer, 3);

		buffer[0] = 0x01;
		buffer[1] = 0x03;

		printf("Befehl oder Abbrechen (-1)?\n");
		read(0, stdBuffer, 3);

		int number = atoi(stdBuffer);
		if(number == -1) {
			end = 1;
		} else {
			buffer[2] = atoi(stdBuffer);

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
