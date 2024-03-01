SPP_COMM=../../comm
SPP_COMM_TBASE=../../comm/tbase
SPP_ASYNC=../../async
SPP_ASYNC_FRAME=../../async_frame
MT_LIB=../micro_thread

INCCOMM=-I$(MT_LIB) -I$(SPP_COMM) -I$(SPP_COMM_TBASE) -I$(SPP_ASYNC) -I$(SPP_ASYNC_FRAME) -I../../worker -I../../comm/stat_mgr
LIBCOMM=-L$(MT_LIB) -lmt

DEBUG= -g

BINARY= libmtspp.so


# Comment the following line if you are not using the gnu c compiler
#C_ARGS = -Wall -g -D_DEBUG
C_ARGS = -Wall -ggdb -DMD_HAVE_EPOLL
#.SUFFIXES: .o .cpp

# You might have to change this if your c compiler is not cc
CC = g++
C_ARGS= -g -fPIC 

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You shouldn't need to make any more changes below this line.

#.c.o:
#	$(CC) -c $*.c

all:  $(BINARY)  

.cpp.o:
	    $(CC) $(C_ARGS) $(INCCOMM) -c $^ 
.c.o:
	    gcc $(C_ARGS)  $(INCCOMM) -c $^
					  
clean:
	rm -f $(BINARY)  *.o *.gch
	#-rm ar.tmp tmp-libsyncframe.a

libmtspp.a : SyncFrame.o
	rm -f $@
	ar q  $@  $^ 
	echo OPEN $@ > ar.tmp
	echo ADDLIB $(MT_LIB)/libmt.a >> ar.tmp
	echo SAVE >> ar.tmp
	echo END >> ar.tmp
	ar -M < ar.tmp
	rm ar.tmp

libmtspp.so : SyncFrame.o
	@echo -e  Linking $@ ...
	@rm -f $@
	@$(CC) -o $@ $^ -shared $(C_ARGS) -Wl,-z,interpose
