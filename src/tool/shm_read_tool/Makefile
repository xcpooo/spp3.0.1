ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

shmread.o:shmread.c
	gcc -g -c shmread.c -o shmread.o $(CFLAGS)
shmread:shmread.o
	gcc shmread.o -o shmread $(CFLAGS)
	install shmread  ../../../bin/
clean:
	rm -rf *.o shmread
all:shmread

