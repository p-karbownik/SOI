__start__: vsfs.out
	./vsfs.out 1

vsfs.out: main.o vsfs.o
	gcc main.o vsfs.o -o vsfs.out

main.o: main.c vsfs.h
	gcc main.c -c -o main.o

vsfs.o: vsfs.h vsfs.c errors.h
	gcc vsfs.c -c -o vsfs.o

remake : clean run

