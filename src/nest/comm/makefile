FRAME_BASE = ../../
FRAME_COMM = $(FRAME_BASE)/comm
FRAME_TBASE = $(FRAME_COMM)/tbase
FRAME_SOCK  = $(FRAME_TBASE)/tsockcommu
FRAME_ASYNC = $(FRAME_BASE)/async_frame
PB_BASE     = ../exlib/pb

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif  

CFLAGS += -O2 -g -Wall -D_GNU_SOURCE -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL 
INC += -I$(FRAME_COMM) -I$(FRAME_TBASE) -I$(FRAME_SOCK) -I$(FRAME_ASYNC) -I$(PB_BASE)

BIN = libnestcomm.a
 
all : proto $(BIN)

### Proto file process
PB_FILES = $(wildcard *.proto)
PB_SRCS  = $(patsubst %.proto,%.pb.cc,$(PB_FILES))
PB_OBJS  = $(patsubst %.proto,%.pb.o,$(PB_FILES))

proto : $(PB_SRCS)  $(PB_OBJS)

%.pb.cc : %.proto
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(PB_BASE)/protoc $< --cpp_out=./  $(CRESET)

OBJ = weight_schedule.o  nest_context.o  nest_channel.o  nest_commu.o  nest_proto.o  nest.pb.o  nest_timer.o  proc_comm.o

libnestcomm.a : $(OBJ) 
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@ar crs  $@  $^  $(CRESET)
	@chmod +x $@

include $(FRAME_BASE)/incl_comm.mk
