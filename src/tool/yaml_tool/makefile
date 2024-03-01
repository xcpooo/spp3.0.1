YAMLCPP = yamlcpp
TINYXML = ../../comm/tinyxml

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
	LIB += -lyamlcpp32
else
	LIB += -lyamlcpp64
endif

CFLAGS += -g -Wall -D_GNU_SOURCE -DTIXML_USE_STL -Wno-write-strings -Werror
INC += -I$(YAMLCPP)/include -I$(TINYXML) 
LIB += -L$(YAMLCPP) -Wl,-rpath,$(LIB_PATH)

TARGET=yaml_tool
all : $(TARGET)
OBJ = $(TINYXML)/tinyxml.o \
	$(TINYXML)/tinyxmlerror.o \
	$(TINYXML)/tinyxmlparser.o \
	$(TINYXML)/tinystr.o \
	Main.o			\
	Megatron.o \

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
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	install $(TARGET) ../../../bin/
%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
clean:
	@rm -f $(OBJ) $(TARGET)

