DEST=libattr_api.a
SOURCE=$(wildcard *.c)
OBJ=$(patsubst %.c, %.o, $(SOURCE))

CFLAGS= -g -Wall
CC=gcc

all:$(DEST) 

$(DEST):$(OBJ)
	ar -rs $(DEST) $(OBJ)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< -I./inner_inc

clean:
	rm -rf $(DEST) *.o

install:
	cp *.h ../include/
	cp *.a ../lib/
