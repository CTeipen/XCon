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

	buffer[3] = 0x01;

	write(fd, buffer, SIZE);

	close(fd);
	printf("Close..\n");

	return 0;

}
