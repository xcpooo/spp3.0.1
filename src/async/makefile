COMM = ../comm
CONFBASE = ../comm/tconfbase
TBASE = ../comm/tbase
TSOCKCOMMU = ../comm/tbase/tsockcommu
PROXY= ../proxy
WORKER=../worker
ASYNC=../async
LIB_PATH=../../bin/lib/


ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

CFLAGS += -O2 -g -ggdb -Wall -D_GNU_SOURCE -Wno-write-strings -Werror 
CFLAGS += -pipe -fno-ident -fPIC -shared -z defs
INC += -I./ -I$(COMM) -I$(CONFBASE) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(ASYNC)
LIB += -lpthread -ldl -rdynamic -L$(COMM) -L$(ASYNC)/ -lcommon -Wl,-rpath,$(LIB_PATH)
TARGET=libasync_epoll.so
all : $(TARGET)
OBJ = client_unit.o \
	general_session.o \
	tm_session.o \
	session_mgr.o \
	global.o \
	interface.o \
	spp_async_interface.o

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

$(TARGET):$(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ -shared $(CFLAGS) $(LIB) $(CRESET)
	install *.so ../../bin/lib/

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)   
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)    
clean:
	@rm -f $(OBJ) $(TARGET)

