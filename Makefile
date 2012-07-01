CC = clang++
CFLAGS = -std=c++11 -Wall -g -O4
LDFLAGS = -lcrypto -lboost_filesystem -lboost_system -lsqlite3
includes = $(wildcard ./*.hpp)

all: mescalero

mescalero: mescalero.o misc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.cpp.o: ${includes}
	$(CC) $(CFLAGS) -c $<

#all:
#	clang++ -g -std=c++11 -Wall -O0 mescalero.cpp -o mescalero -lcrypto -lboost_filesystem -lboost_system -lsqlite3


.PHONY: clean

clean:
	rm -f mescalero *.o
