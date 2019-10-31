.PHONY : clean test
.DEFAULT_GOAL : make

CC = gcc
CFLAGS = -g -Wall -o3 -std=gnu99

all : clean utils list hashtable htiterator server os client

clean :
	-rm -f server
	-rm -f objstore.sock
	-rm -f client
	-rm -f libs/libList.a
	-rm -f libs/libUtils.a
	-rm -f libs/libHashTable.a
	-rm -f libs/libHTIterator.a
	-rm -f libs/libOS.a
	-rm -f testout.log
	-rm -f status.dat
	-rm -f -r data/*

utils :
	$(CC) $(CFLAGS) libs/libUtils.c -c -o libs/libUtils.o
	ar rvs libs/libUtils.a libs/libUtils.h libs/libUtils.o
	-rm -f libs/libUtils.o

list :
	$(CC) $(CFLAGS) libs/libList.c -c -o libs/libList.o
	ar rvs libs/libList.a libs/libList.h libs/libList.o
	-rm -f libs/libList.o

hashtable :
	$(CC) $(CFLAGS) libs/libList.c -c -o libs/libList.o
	$(CC) $(CFLAGS) libs/libHashTable.c -c -o libs/libHashTable.o
	ar rvs libs/libHashTable.a libs/libHashTable.h libs/libHashTable.o libs/libList.o libs/libList.h
	-rm -f libs/libHashTable.o
	-rm -f libs/libList.o

htiterator : 
	$(CC) $(CFLAGS) libs/libList.c -c -o libs/libList.o
	$(CC) $(CFLAGS) libs/libHashTable.c -c -o libs/libHashTable.o
	$(CC) $(CFLAGS) libs/libHTIterator.c -c -o libs/libHTIterator.o
	ar rvs libs/libHTIterator.a libs/libHTIterator.o libs/libHTIterator.h libs/libHashTable.h libs/libHashTable.o libs/libList.o libs/libList.h
	-rm -f libs/libHashTable.o
	-rm -f libs/libList.o
	-rm -f libs/libHTIterator.o

os :
	$(CC) $(CFLAGS) libs/libUtils.c -c -o libs/libUtils.o
	$(CC) $(CFLAGS) libs/libOS.c -c -o libs/libOS.o
	ar rvs libs/libOS.a libs/libOS.h libs/libOS.o libs/libUtils.o libs/libUtils.h
	-rm -f libs/libUtils.o
	-rm -f libs/libOS.o

server : utils hashtable htiterator
	$(CC) $(CFLAGS) server.c -o server -L libs -lUtils -lHashTable -lHTIterator -lpthread

client : os utils
	$(CC) $(CFLAGS) client.c -o client -L libs -lOS -lUtils

test :
	./test.sh
	./testsum.sh
