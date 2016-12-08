#include <fcntl.h> 
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 20

int main(int argc, char *argv[]) {

	
	char* pressed = malloc(100*sizeof(char));
	int mode = 0;

	do{

		pressed = "";
		int fd = open(argv[1], O_RDWR);

		if (fd == -1) {
			/* An error occurred. Print an error message and bail.  */ 
			perror ("open"); 

			return 1; 
		} 

		char* buffer = malloc(SIZE);

		int len = read(fd, buffer, SIZE);

		if(len < 0){

			perror("Read: ");
			exit(-1);

		}

		//printf("Read len: %d, Read Output: %s\n", len, buffer);

		int i = 0;
		char* type[14];

		type[0] = "0x00\t";
		type[1] = "size of report";
		type[2] = "\nUp, Down, Left, Right, Start, Back, Left Stick, Right Stick";
		type[3] = "LB, RB, XBOX, A, B, X, Y";
		type[4] = "\nLT";
		type[5] = "RT";

		type[6] = "\nleft stick x - l\t";
		type[7] = "\t     - r\t";

		type[8] = "left stick y - b\t";
		type[9] = "\t     - t\t";		

		type[10] = "\nright stick x - l\t";
		type[11] = "\t      - r\t";

		type[12] = "right stick y - b\t";
		type[13] = "\t      - t\t";		


		while(i < 14){		

			if(i == 2 && pressed == ""){

				switch (buffer[i]){

					case 0: 	pressed = "";
								break;

					case 1: 	pressed = "D-Pad UP";
								break;

					case 2: 	pressed = "D-Pad DOWN";
								break;

					case 4: 	pressed = "D-Pad LEFT";
								break;

					case 8: 	pressed = "D-Pad RIGHT";
								break;

					case 16: 	pressed = "Start";
								break;

					case 32: 	pressed = "Back";
								break;

					case 64: 	pressed = "Left Stick";
								break;

					default: 	pressed = "Right Stick";
								break;

				}

			}

			if(i == 3 && pressed == ""){
				
				switch (buffer[i]){
					case 0: 	pressed = "";
								break;

					case 1: 	pressed = "LB";
								break;

					case 2: 	pressed = "RB";
								break;

					case 4: 	pressed = "XBOX-Button";
								break;

					case 16: 	pressed = "A";
								break;

					case 32: 	pressed = "B";
								break;

					case 64: 	pressed = "X";
								break;

					default: 	pressed = "Y";
								break;

				}

			}

			if(i == 4 && pressed == "" && buffer[i] > 0 && buffer[i] < 120){			
				pressed = "LT";

			}

			if(i == 5 && pressed == "" && buffer[i] > 0 && buffer[i] < 120){
				pressed = "RT";

			}
	
			if(mode == 0)			
				printf("%s\t:\t%u\n", type[i], buffer[i]);
		
			i++;

		}

		
		if(mode == 0)
			printf("\n-------------------------\n");
		
		if(pressed != "")
			printf("Pressed: %s\n", pressed);
			

		close(fd);
		free(buffer);

	}while(1);

	free(pressed);

	return 0;

}
