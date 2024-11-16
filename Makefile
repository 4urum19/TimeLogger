CC = g++

CompileParms = -c -Wall -std=c++17 -O2

OBJS = gitwrapper.o

Opdr: $(OBJS)
	$(CC) $(OBJS) -o git

clean:
	rm -f *.o git

gitwrapper.o: gitwrapper.cpp
	$(CC) $(CompileParms)  gitwrapper.cpp

