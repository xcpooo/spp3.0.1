COMM = ../comm
TBASE = ../comm/tbase
PROXY= ../proxy
WORKER=../worker
ASYNC=../async
ASYNCFRAME=../async_frame
STATMGR=../comm/stat_mgr
MT=../sync_frame/micro_thread
SYNCFRAME=../sync_frame/spp_plugin
LIB_PATH=../bin/lib/
SYNC=../sync
SEGV_DIR = ../external/segv

L5_LIB = $(ASYNCFRAME)/ex_lib/L5api

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif  

CFLAGS += -g -Wall -D_GNU_SOURCE -D_SPP_WORKER -D_MP_MODE -Wno-write-strings -DTIXML_USE_STL -Werror #-DOPEN_PRINT #定义_MP_MODE使得多进程写日志安全
INC += -I$(COMM) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(ASYNC) -I$(SYNCFRAME) -I$(STATMGR) -I$(SEGV_DIR)
LIB += -lpthread -ldl -rdynamic -L../nest/comm -lnestcomm -L$(COMM) -L$(ASYNC) -L$(ASYNCFRAME) -L$(SYNC) \
	-L$(SYNCFRAME) -L$(MT) -L$(L5_LIB) -L$(SEGV_DIR) -lcommon -lsppasync -lasync_epoll -lmtspp -lmt -lqos_client -lsegv -Wl,-rpath,$(LIB_PATH)

TARGET=spp_worker 
all : $(TARGET)
OBJ = main.o			\
	defaultworker.o	\
	$(COMM)/timerlist.o \
	$(COMM)/poller.o \
	$(STATMGR)/ICostStat.o
include ../incl_comm.mk
