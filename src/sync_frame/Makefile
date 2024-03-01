
# You might have to change this if your c compiler is not cc
CC = g++
C_ARGS=-Wall -g -fPIC

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You shouldn't need to make any more changes below this line.
.cpp.o:
	    $(CC) $(C_ARGS) $(INCCOMM) -c $^ 
.c.o:
	    gcc $(C_ARGS)  $(INCCOMM) -c $^
					  
# Targets, maybe no use spp plugin, don't make it
all: mtlib spp_plug install

# Defines
MT_LIB=./micro_thread
SYNC_LIB=./spp_plugin

mtlib:
	make -C $(MT_LIB)

spp_plug:
	make -C $(SYNC_LIB)

clean:
	make -C $(MT_LIB) clean
	make -C $(SYNC_LIB) clean
	-rm -rf ./include
	-rm -rf ./lib

install:
	mkdir -p ./include
	cp  $(MT_LIB)/mt_api.h $(MT_LIB)/mt_version.h $(MT_LIB)/mt_msg.h $(MT_LIB)/mt_incl.h $(SYNC_LIB)/*.h ./include
	install $(MT_LIB)/*.so ../../bin/lib
	install $(SYNC_LIB)/*.so ../../bin/lib
	mkdir -p ../module/Example/sync/mt_alone/lib
	install $(MT_LIB)/*.so ../module/Example/sync/mt_alone/lib
