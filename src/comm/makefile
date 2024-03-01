# TINYXML_USE_STL can be used to turn on STL support. NO, then STL
# will not be used. YES will include the STL files.
TINYXML_USE_STL := YES

#****************************************************************************

COMM = ../comm
TBASE = ../comm/tbase
TSOCKCOMMU = ../comm/tbase/tsockcommu
MONITOR = ../comm/attr_api
LIB_PATH = ../../bin/lib/
TINYXML = ./tinyxml
TCONFBASE = ./tconfbase
STATMGR = ./stat_mgr
REPORT = ./report
BUILD_DIR=../comm
ULS_DIR=../nest/exlib/uls
CUR_DIR=$(shell pwd)

ifeq ($(ARCH),32)
	CFLAGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
	ULS_LIB = liblogsys_client_32.a
else
	CFLAGS +=  -m64 -DSUS_LINUX -pthread
	ULS_LIB = liblogsys_client_64.a
endif

CFLAGS += -O2 -fno-inline -g -ggdb -Wall -D_GNU_SOURCE -D_MP_MODE -Wno-write-strings #-Werror 
CFLAGS += -pipe -fno-ident -fPIC -shared -z defs
ifeq (YES, ${TINYXML_USE_STL})
	CFLAGS += -DTIXML_USE_STL
endif

INC += -I./ -I$(COMM) -I$(TBASE) -I$(TSOCKCOMMU) -I$(ULS_DIR) -I$(TINYXML) -I$(MONITOR)/inner_inc
LIB += -lpthread -ldl
TARGET=libcommon.a
all : $(TARGET)
OBJ = $(shell cd $(ULS_DIR); ar -x $(ULS_LIB); cd $(CUR_DIR); ls $(ULS_DIR)/*.o)
OBJ += $(COMM)/benchadapter.o \
	  $(COMM)/timerlist.o \
	  $(COMM)/frame.o \
	  $(COMM)/global.o \
	  $(COMM)/IPC.o \
	  $(COMM)/poller.o \
	  $(COMM)/serverbase.o \
	  $(COMM)/crc32.o \
	  $(COMM)/keygen.o \
	  $(COMM)/exception.o \
	  $(COMM)/monitor.o \
	  $(TBASE)/tlog.o \
	  $(TBASE)/misc.o \
	  $(TBASE)/tprocmon.o \
	  $(TBASE)/tshmcommu.o \
	  $(TBASE)/tstat.o \
	  $(TBASE)/notify.o \
	  $(TBASE)/time_convert.o \
	  $(TSOCKCOMMU)/tcache.o \
	  $(TSOCKCOMMU)/tconnset.o \
	  $(TSOCKCOMMU)/tepollflow.o \
	  $(TSOCKCOMMU)/tmempool.o \
	  $(TSOCKCOMMU)/tsockcommu.o \
	  $(TSOCKCOMMU)/tsocket.o \
	  $(MONITOR)/Attr_API.o \
	  $(TINYXML)/tinystr.o \
	  $(TINYXML)/tinyxml.o \
	  $(TINYXML)/tinyxmlerror.o \
	  $(TINYXML)/tinyxmlparser.o \
	  $(TCONFBASE)/workerconf.o \
	  $(TCONFBASE)/ctrlconf.o \
	  $(TCONFBASE)/loadconfbase.o \
	  $(TCONFBASE)/loadxml.o \
	  $(TCONFBASE)/moduleconfbase.o \
	  $(TCONFBASE)/proxyconf.o \
	  $(TCONFBASE)/tinyxmlcomm.o \
	  $(TCONFBASE)/sessionmgrconf.o \
	  $(STATMGR)/ICostStat.o \
	  $(STATMGR)/IFrameStat.o \
	  $(STATMGR)/StatMgr.o \
	  $(STATMGR)/StatMgrInstance.o \
	  $(REPORT)/report_handle.o \
	  $(REPORT)/loop_queue.o \
	  $(REPORT)/shm_queue.o \


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
	@ar crs $@ $^ $(CRESET)
	@chmod +x $@

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)   
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)    
clean:
	@rm -f $(OBJ) $(TARGET)

