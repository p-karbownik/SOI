__start__: a.out
	./a.out

a.out: main.o Buffer.o 
	g++ -pthread main.o Buffer.o 

main.o: main.cpp Buffer.h monitor.h
	g++ -c -pthread main.cpp Buffer.h monitor.h

Buffer.o: Buffer.h Buffer.cpp monitor.h
	g++ -c -pthread Buffer.h Buffer.cpp monitor.h
