DEST=libreport.a
SOURCE=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp, %.o, $(SOURCE))

CFLAGS= -g -Wall
CC=g++

all:$(DEST) 

$(DEST):$(OBJ)
	ar -rs $(DEST) $(OBJ)

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $< -I./

clean:
	rm -rf $(DEST) *.o
