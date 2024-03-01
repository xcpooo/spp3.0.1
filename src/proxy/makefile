COMM = ../comm
TBASE = ../comm/tbase
PROXY= ../proxy
WORKER=../worker
ASYNC=../async
ASYNCFRAME=../async_frame
MT=../sync_frame/micro_thread
SYNCFRAME=../sync_frame/spp_plugin
STATMGR=../comm/stat_mgr
LIB_PATH=../bin/lib/
L5_LIB = $(ASYNCFRAME)/ex_lib/L5api

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

CFLAGS += -g -Wall -D_GNU_SOURCE -D_SPP_PROXY -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL#-DOPEN_PRINT
INC += -I$(COMM) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(STATMGR)
LIB += -lpthread -ldl -rdynamic -L../nest/comm -lnestcomm -L$(COMM) -L$(ASYNC) -L$(ASYNCFRAME) \
       -L$(SYNCFRAME) -L$(MT) -L$(L5_LIB) -lcommon -lsppasync -lasync_epoll -lmtspp -lmt -lqos_client -Wl,-rpath,$(LIB_PATH)
  
TARGET=spp_proxy
all : $(TARGET)
OBJ = main.o			\
	defaultproxy.o \
	$(STATMGR)/ICostStat.o \
	$(STATMGR)/IFrameStat.o
include ../incl_comm.mk
