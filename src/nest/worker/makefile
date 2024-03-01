FRAME_BASE  = ../../
FRAME_COMM  = $(FRAME_BASE)/comm
FRAME_TBASE = $(FRAME_COMM)/tbase
FRAME_SOCK  = $(FRAME_TBASE)/tsockcommu
FRAME_ASYNC = $(FRAME_BASE)/async_frame
FRAME_POLL  = $(FRAME_BASE)/async
FRAME_EXT   = $(FRAME_BASE)/external

L5_LIB      = $(FRAME_ASYNC)/ex_lib/L5api
SEGV_LIB    = $(FRAME_EXT)/segv
STATMGR     = $(FRAME_COMM)/stat_mgr
LIB_PATH    = ../bin/lib/

SYNC_FRAME  = $(FRAME_BASE)/sync_frame
MT	    = $(SYNC_FRAME)/micro_thread
SYNCFRAME   = $(SYNC_FRAME)/spp_plugin

RED = \\e[1m\\e[31m
DARKRED = \\e[31m 
GREEN = \\e[1m\\e[32m
DARKGREEN = \\e[32m 
BLUE = \\e[1m\\e[34m
DARKBLUE = \\e[34m 
YELLOW = \\e[1m\\e[33m
DARKYELLOW = \\e[33m 
MAGENTA = \\e[1m\\e[35m
DARKMAGENTA = \\e[35m 
CYAN = \\e[1m\\e[36m
DARKCYAN = \\e[36m 
RESET = \\e[m
CRESET =  ;echo -ne \\e[m; test -s $@

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
	PB_LIB = -L../exlib/pb -lpb32
else
	CFLAGS += -m64
	PB_LIB = -L../exlib/pb -lpb64
endif

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)   
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)    
%.pb.o:%.pb.cc
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)  

CFLAGS += -g -Wall -D_GNU_SOURCE -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL 
INC += -I$(FRAME_COMM) -I$(FRAME_COMM)/tconfbase -I$(FRAME_TBASE) -I$(FRAME_SOCK) -I$(FRAME_ASYNC) \
       -I$(STATMGR) -I$(FRAME_POLL) -I../comm -I../exlib/pb  -I$(SYNC_FRAME)/include -I$(FRAME_EXT)/segv
LIB += -L../comm -lnestcomm  -rdynamic -L$(FRAME_ASYNC) -lsppasync -L$(FRAME_POLL) -lasync_epoll -L$(FRAME_COMM) -lcommon \
	-L$(SYNCFRAME) -lmtspp -L$(MT) -lmt -L$(L5_LIB) -lqos_client -L$(SEGV_LIB) -lsegv -lpthread -ldl  -Wl,-rpath,$(LIB_PATH) $(PB_LIB)

BIN = nest_worker
 
all : $(BIN)

OBJ = nest_worker.o  main.o \
	$(STATMGR)/ICostStat.o $(STATMGR)/IFrameStat.o

nest_worker : $(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)

clean:
	@echo "Cleaning..."
	@rm -f $(OBJ) $(BIN)
