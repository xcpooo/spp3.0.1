########MAKEFILE##########
DEBUG= -g

BINARY = libmt.so

# Comment the following line if you are not using the gnu c compiler
#C_ARGS = -Wall -g -fPIC -D_DEBUG
C_ARGS = -Wall -g -fPIC -O1 -DMT_REAL_TIME
#.SUFFIXES: .o .cpp

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You might have to change this if your c compiler is not cc
CC = g++

# You shouldn't need to make any more changes below this line.
INCCOMM=-I../valgrind

#.c.o:
#	$(CC) -c $*.c

all:	$(BINARY) 

### shell color config
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

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)

%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)  

%.o: %.S
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET) 

clean:
	@rm -f $(BINARY)  *.o mt_self_echo  


LIB_OBJ = micro_thread.o epoll_proxy.o arch_ctx.o mt_session.o mt_notify.o mt_action.o mt_mbuf_pool.o mt_api.o\
	 mt_connection.o mt_concurrent.o mt_sys_hook.o heap_timer.o  mt_cache.o  mt_net.o

libmt.a: $(LIB_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@ar crs  $@  $^  $(CRESET)
	@chmod +x $@ 

libmt.so: $(LIB_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@$(CC) -o $@ $^ -shared $(C_ARGS) -Wl,-z,interpose $(CRESET)

mt_self_echo: mt_self_echo.o libmt.a
	@echo -e Compile $(CYAN)$@$(RESET) ...$(RED)
	@$(CC) -o $@ $^ -lstdc++ -ldl $(CRESET)
	

