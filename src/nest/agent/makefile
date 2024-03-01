FRAME_BASE  = ../../
FRAME_COMM  = $(FRAME_BASE)/comm
FRAME_TBASE = $(FRAME_COMM)/tbase
FRAME_SOCK  = $(FRAME_TBASE)/tsockcommu
FRAME_ASYNC = $(FRAME_BASE)/async_frame
FRAME_POLL  = $(FRAME_BASE)/async
L5_LIB      = $(FRAME_ASYNC)/ex_lib/L5api

STATMGR     = $(FRAME_COMM)/stat_mgr
LIB_PATH    = ../bin/lib/

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
	CFLAGS += -m64 -fno-rtti
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
INC += -I$(FRAME_COMM) -I$(FRAME_COMM)/tconfbase -I$(FRAME_TBASE) -I$(FRAME_SOCK) -I$(FRAME_ASYNC) -I../comm -I../exlib/pb
LIB += -L../comm -lnestcomm  -L$(FRAME_ASYNC) -lsppasync -L$(FRAME_POLL) -lasync_epoll -L$(FRAME_COMM) -lcommon \
	-L$(L5_LIB) -lqos_client -lpthread -ldl  -Wl,-rpath,$(LIB_PATH) $(PB_LIB)

BIN = nest_agent nest_tool agent_exec_tool nest_start_tool nest_clone
 
all : $(BIN)

OBJ = agent_process.o  sys_info.o  agent_net_mng.o  nest_agent.o  agent_msg.o  agent_cgroups.o agent_thread.o  agent_conf.o  main.o\
	$(STATMGR)/ICostStat.o $(STATMGR)/IFrameStat.o

nest_agent : $(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)

nest_tool : nest_test.o ../comm/nest.pb.o  ../comm/nest_proto.o
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@$(CXX) -o $@ $^ $(CFLAGS)  $(PB_LIB) -lpthread -ldl   $(CRESET)

nest_start_tool : nest_start_tool.o
	@echo -e Linking $(CYAN)$@$(RESET) ...$(RED)
	@$(CXX) -o $@ $^ $(CFLAGS)  $(CRESET)

agent_exec_tool : agent_tool.o
	@echo -e Linking $(CYAN)$@$(RESET) ...$(RED)
	@$(CXX) -o $@ $^ $(CFLAGS)  $(CRESET)

nest_clone : nest_clone.o sys_info.o agent_net_dispatch.o agent_cgroups.o  agent_conf.o $(STATMGR)/ICostStat.o $(STATMGR)/IFrameStat.o
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)

clean:
	@echo "Cleaning..." 
	@rm -rf $(OBJ)
	@rm -f $(BIN)
	@rm -rf *.o
