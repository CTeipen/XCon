#include <fcntl.h> 
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 6

int main(int argc, char *argv[]) {

	int fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		/* An error occurred. Print an error message and bail.  */ 
		perror ("open"); 

		return 1; 
	} 

	char* buffer = calloc(SIZE, sizeof(char));

	buffer[0] = 0x00;
	buffer[1] = 0x06;
	buffer[2] = 0x01;
	buffer[3] = 0xff;
	buffer[4] = 0x01;
	buffer[5] = 0xff;

	write(fd, buffer, SIZE);

	close(fd);
	printf("Close..\n");

	return 0;

}
