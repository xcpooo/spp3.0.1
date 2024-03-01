#gcc version
CC  = gcc
CXX = g++
GCCVER := $(shell $(CC) -dumpversion | awk -F. '{ print $$1"."$$2}' )

L5_LIB=./ex_lib/L5api

INC += -I../comm -I../comm/tbase -I../async -I../worker -I../comm/stat_mgr
LIB += -lpthread -ldl -rdynamic 
INC += -I$(L5_LIB)

BUILD_DIR=../../build 
OBJ_DIR=./obj
LIB_DIR=.

LIB_NAME=libsppasync.so


TARGET = $(LIB_DIR)/$(LIB_NAME)

	
OBJ = $(OBJ_DIR)/ActionInfo.o \
      $(OBJ_DIR)/ActionSet.o \
      $(OBJ_DIR)/AsyncFrame.o \
      $(OBJ_DIR)/NetHandler.o \
      $(OBJ_DIR)/NetMgr.o \
      $(OBJ_DIR)/MsgBase.o \
      $(OBJ_DIR)/NonStateActionInfo.o \
      $(OBJ_DIR)/StateActionInfo.o \
      $(OBJ_DIR)/AntiParalActionInfo.o \
      $(OBJ_DIR)/RouteMgr.o \
      $(OBJ_DIR)/ParallelNetHandler.o \
      $(OBJ_DIR)/SEXInterface.o


OBJ_EXT= .o
CXXSRC_EXT= .cpp
CSRC_EXT= .c

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif
CFLAGS  += -O2 -g -fPIC -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRNT

GREEN = \\e[1m\\e[32m
CYAN = \\e[1m\\e[36m
RESET = \\e[m

#CXXFLAGS=-fno-rtti
CXXFLAGS+=$(CFLAGS)


$(OBJ_DIR)/%$(OBJ_EXT): %$(CXXSRC_EXT)
	@echo -e "Compiling $(GREEN)$<$(RESET)..."
	@-mkdir -p $(OBJ_DIR)
	@$(CXX) $(INC) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%$(OBJ_EXT): %$(CSRC_EXT)
	@echo -e "Compiling $(GREEN)$<$(RESET)..."
	@-mkdir -p $(OBJ_DIR)
	@$(CC) $(INC) $(CFLAGS) -c $< -o $@


all:$(TARGET) lib_so

lib_so:
ifeq ($(ARCH),32)
	@install $(L5_LIB)/libqos_client32.so $(L5_LIB)/libqos_client.so
else
	@install $(L5_LIB)/libqos_client64.so $(L5_LIB)/libqos_client.so	
endif
	install $(L5_LIB)/libqos_client.so ../../bin/lib

$(TARGET) : $(OBJ)
	@echo -e "Linking $(CYAN)$@$(RESET) ..."
	@-mkdir -p $(LIB_DIR)
	@$(CXX) -o $@ $^ -shared $(CXXFLAGS) $(LIB)
	install *.so ../../bin/lib

clean:
	@echo
	@echo "Cleaning..." 
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)	
	rm -f $(L5_LIB)/libqos_client.so

