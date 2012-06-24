

all:
	clang++ -std=c++11 -Wall -O4 mescalero.cpp -o mescalero -lcrypto -lboost_filesystem -lboost_system -lsqlite3


.PHONY: clean

clean:
	rm -f mescalero *.o
