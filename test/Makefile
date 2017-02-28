main: prog-r prog-w_l prog-w_v

prog-w_l: write_l.o
	gcc -o prog-w_l write_l.o

prog-w_v: write_v.o
	gcc -o prog-w_v write_v.o

prog-r: read.o
	gcc -o prog-r read.o

read.o: read.c
	gcc -c read.c

write_l.o: write_l.c
	gcc -c write_l.c

write_v.o: write_v.c
	gcc -c write_v.c

clean:
	rm -rf prog-r read.o prog-w write.o
