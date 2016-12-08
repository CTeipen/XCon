main: prog-r prog-w

prog-w: write.o
	gcc -o prog-w write.o

prog-r: read.o
	gcc -o prog-r read.o

read.o: read.c
	gcc -c read.c

write.o: write.c
	gcc -c write.c

clean:
	rm -rf prog-r read.o prog-w write.o
