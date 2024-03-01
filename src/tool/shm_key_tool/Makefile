COMM = ../../comm
CONFBASE = ../../comm/tconfbase
TBASE = ../../comm/tbase

INC = -I$(COMM) -I$(CONFBASE) -I$(TBASE)
LIB = -L$(COMM)  -lcommon

ifeq ($(ARCH),32)
	    CFLAGS += -m32 -march=pentium4
endif

all:shm_key_tool
shm_key_tool:main.o
	    g++ $(CFLAGS) $(INC)  main.o -g -o shm_key_tool -Wno-write-strings $(LIB)
		install shm_key_tool ../../../bin/
main.o:main.cpp
	    g++ $(CFLAGS) $(INC)  main.cpp -c -g -Wno-write-strings
clean:
	    @-rm *.o shm_key_tool -rf
