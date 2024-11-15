CC = g++

CompileParms = -c -Wall -std=c++17 -O2

OBJS = main.o

Opdr: $(OBJS)
	$(CC) $(OBJS) -o git

clean:
	rm -f *.o git

main.o: main.cpp
	$(CC) $(CompileParms)  main.cpp

