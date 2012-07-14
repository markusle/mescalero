CC = g++
CPPFLAGS = -DSQLITE_HAS_CODEC
INCLUDE = -I/home/markus/programming/cpp/sqlcipher/sqlcipher
CFLAGS = -std=c++11 -Wall -O4 
LDFLAGS = -lcrypto -lboost_filesystem -lboost_system -lpthread
CRYPTO_SQLITE = /home/markus/programming/cpp/sqlcipher/sqlcipher/sqlcipher.o
includes = $(wildcard ./*.hpp)

all: mescalero

mescalero: cmdlineParser.o database.o mescalero.o misc.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CRYPTO_SQLITE)

.cpp.o: ${includes}
	$(CC) $(INCLUDE) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f mescalero *.o
