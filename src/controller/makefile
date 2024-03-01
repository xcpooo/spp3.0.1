COMM = ../comm
TBASE = ../comm/tbase
PROXY= ../proxy
WORKER=../worker
STAT_MGR=../comm/stat_mgr
LIB_PATH = ./lib

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif   

CFLAGS += -g  -Wall -rdynamic -D_GNU_SOURCE -D_SPP_CTRL  -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL#-DOPEN_PRINT
INC += -I$(COMM) -I$(TBASE) -I$(STAT_MGR)
LIB += -lpthread -ldl -rdynamic -L$(COMM) -lcommon -Wl,-rpath,$(LIB_PATH)
TARGET=spp_ctrl 
all : $(TARGET)
OBJ = main.o			\
	defaultctrl.o
include ../incl_comm.mk	
