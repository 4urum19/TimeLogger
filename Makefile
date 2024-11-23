CC = g++

CompileParms = -c -Wall -std=c++17 -O2

GIT_OBJS = gitwrapper.o
TL_OBJS = TimeLogger.o

all: git TimeLogger

git: $(GIT_OBJS)
	$(CC) $(GIT_OBJS) -o git

TimeLogger: $(TL_OBJS)
	$(CC) $(TL_OBJS) -o TimeLogger

clean:
	rm -f *.o git TimeLogger

gitwrapper.o: gitwrapper.cpp
	$(CC) $(CompileParms) gitwrapper.cpp

TimeLogger.o: TimeLogger.cpp
	$(CC) $(CompileParms) TimeLogger.cpp