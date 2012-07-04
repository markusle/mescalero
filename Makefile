CC = clang++
CFLAGS = -std=c++11 -Wall -g -O4
LDFLAGS = -lcrypto -lboost_filesystem -lboost_system -lsqlite3
includes = $(wildcard ./*.hpp)

all: mescalero

mescalero: database.o mescalero.o misc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.cpp.o: ${includes}
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f mescalero *.o
