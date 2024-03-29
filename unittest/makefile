# By aoxu, 2010-10-19
# SYNOPSIS:
#
#   make [all]  - makes everything.
#   make TARGET - makes the given target.
#   make clean  - removes all files generated by make.

# Please tweak the following variable definitions as needed by your
# project, except GTEST_HEADERS, which you can use in your own targets
# but shouldn't modify.
SRC_DIR = ../src

COMM = $(SRC_DIR)/comm
TBASE = $(SRC_DIR)/comm/tbase
ASYNC = $(SRC_DIR)/async
TSOCKCOMMU = $(SRC_DIR)/comm/tbase/tsockcommu
WORKER = $(SRC_DIR)/worker
PROXY = $(SRC_DIR)/proxy
TINYXML = $(SRC_DIR)/comm/tinyxml
TCONFBASE = $(SRC_DIR)/comm/tconfbase
ASYNC_FRAME = $(SRC_DIR)/async_frame
STAT_MGR = $(SRC_DIR)/comm/stat_mgr

LIB_PATH = ../../bin/lib/

TEST_UTIL = util
TEST_ASYNC_FRAME = async_frame
TEST_LOAD_CONF_BASE = tconfbase
TEST_STAT_MGR = stat_mgr

#单元测试代码添加在这里
OBJ = $(TEST_LOAD_CONF_BASE)/proxyconf_unittest.o \
	  $(TEST_LOAD_CONF_BASE)/workerconf_unittest.o\
	  $(TEST_LOAD_CONF_BASE)/ctrlconf_unittest.o\
	  $(TEST_LOAD_CONF_BASE)/sessionmgrconf_unittest.o\
	$(TEST_ASYNC_FRAME)/atomic_int64_unittest.o \
    $(TEST_ASYNC_FRAME)/ProcessLock_unittest.o \
    $(TEST_ASYNC_FRAME)/NameIdMap_unittest.o \
    $(TEST_ASYNC_FRAME)/ActionInfo_unittest.o \
    $(TEST_ASYNC_FRAME)/NonStateActionInfo_unittest.o \
    $(TEST_ASYNC_FRAME)/StateActionInfo_unittest.o \
    $(TEST_ASYNC_FRAME)/AntiParalActionInfo_unittest.o \
    $(TEST_ASYNC_FRAME)/MsgBase_unittest.o \
    $(TEST_ASYNC_FRAME)/ActionSet_unittest.o \
    $(TEST_ASYNC_FRAME)/AsyncFrame_unittest.o \
    $(TEST_ASYNC_FRAME)/L5HbMap_unittest.o \
    $(TEST_ASYNC_FRAME)/MttcTSMap_unittest.o \
	$(TEST_STAT_MGR)/statmgr_unittest.o\

#单元测试需要的spp模块
OBJ += $(TEST_UTIL)/file_utility.o \
    $(ASYNC_FRAME)/ProcessLock.o\
    $(ASYNC_FRAME)/NameIdMap.o\
    $(ASYNC_FRAME)/ActionInfo.o\
    $(ASYNC_FRAME)/NetMgr.o\
    $(ASYNC_FRAME)/NetHandler.o\
    $(ASYNC_FRAME)/AsyncFrame.o\
    $(ASYNC_FRAME)/MsgBase.o\
    $(ASYNC_FRAME)/ActionSet.o\
    $(ASYNC_FRAME)/L5Handler.o\
    $(ASYNC_FRAME)/L5HbMap.o\
    $(ASYNC_FRAME)/RouteMgr.o\
    $(ASYNC_FRAME)/StaticRoute.o\
    $(ASYNC_FRAME)/MttcTSMap.o\
    $(ASYNC_FRAME)/MttcRule.o\
    $(ASYNC_FRAME)/NonStateActionInfo.o\
    $(ASYNC_FRAME)/StateActionInfo.o\
    $(ASYNC_FRAME)/AntiParalActionInfo.o\
    $(ASYNC_FRAME)/SEXInterface.o\
    $(ASYNC_FRAME)/ParallelNetHandler.o\
    $(TBASE)/tlog.o\
    $(TBASE)/misc.o\
    $(TBASE)/notify.o\
    $(TBASE)/tstat.o\
    $(TBASE)/tprocmon.o\
    $(TBASE)/tshmcommu.o\
    $(COMM)/poller.o\
    $(COMM)/timerlist.o\
    $(COMM)/benchadapter.o\
    $(COMM)/serverbase.o\
    $(COMM)/global.o\
    $(COMM)/keygen.o\
    $(COMM)/crc32.o\
    $(COMM)/frame.o\
    $(TSOCKCOMMU)/tsockcommu.o\
    $(TSOCKCOMMU)/tcache.o\
    $(TSOCKCOMMU)/tconnset.o\
    $(TSOCKCOMMU)/tepollflow.o\
    $(TSOCKCOMMU)/tmempool.o\
    $(TSOCKCOMMU)/tsocket.o\
    $(ASYNC)/spp_async_interface.o\
    $(ASYNC)/client_unit.o\
    $(ASYNC)/general_session.o\
    $(ASYNC)/tm_session.o\
    $(ASYNC)/session_mgr.o\
    $(ASYNC)/interface.o\
    $(ASYNC)/global.o\
    $(TCONFBASE)/tinyxmlcomm.o\
    $(TCONFBASE)/ctrlconf.o\
    $(TCONFBASE)/loadconfbase.o\
    $(TCONFBASE)/loadxml.o\
    $(TCONFBASE)/moduleconfbase.o\
    $(TCONFBASE)/proxyconf.o\
    $(TCONFBASE)/workerconf.o\
    $(TCONFBASE)/sessionmgrconf.o\
    $(PROXY)/defaultproxy.o \
    $(TINYXML)/tinyxml.o\
    $(TINYXML)/tinystr.o \
    $(TINYXML)/tinyxmlerror.o\
    $(TINYXML)/tinyxmlparser.o\
	$(TEST_LOAD_CONF_BASE)/unittesteq.o\
	$(STAT_MGR)/ICostStat.o\
	$(STAT_MGR)/IFrameStat.o\
	$(STAT_MGR)/StatMgrInstance.o\
	$(STAT_MGR)/StatMgr.o\


# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_DIR = ./gtest-1.5.0
INC_GTEST = $(GTEST_DIR)/include
LIB_GTEST = $(GTEST_DIR)/lib
				
CFLAGS += -g -ggdb -Wall -D_GNU_SOURCE -Wno-write-strings -Werror -DTIXML_USE_STL
#加上单元测试宏定义
CFLAGS += -DSPP_UNITTEST

INC += -I./ -I$(TEST_UTIL) -I$(ASYNC) -I$(SRC_DIR) -I$(TBASE) -I$(COMM) -I$(WORKER) -I$(TCONFBASE) -I$(TINYXML) -I$(STAT_MGR) -I$(INC_GTEST) 
#如果需要自己写main函数调用run test，使用LIBGTEST
#如果使用默认main函数调用所有test，使用LIBGTESTMAIN
LIB += -lpthread -ldl -lz $(LIBGTESTMAIN) -L$(LIB_GTEST)
TARGET=spp_unittest

ifeq ($(ARCH),32)
	CFLAGS += -march=pentium4 -m32 -DSUS_LINUX -pthread
	LIBGTEST = -lgtest32
	LIBGTESTMAIN = -lgtest_main32
else
	CFLAGS +=  -m64 -DSUS_LINUX -pthread
	LIBGTEST = -lgtest64
	LIBGTESTMAIN = -lgtest_main64
endif

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

all: $(TARGET) 
	./$(TARGET)

test: $(TARGET)

$(TARGET):$(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
%.o: %.cpp
	@echo -e Compling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)   
%.o: %.c
	@echo -e Compling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)    
clean:
	@rm -f $(OBJ) $(TARGET)
	@rm -f $(TESTS) gtest.a gtest_main.a *.o

