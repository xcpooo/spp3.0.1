CFLAGS=-g
ifeq ($(ARCH),32)
	CFLAGS+=-m32 -march=pentium4
else
endif
INC = -I../../comm -I../../comm/tbase  
TARGET=cost_stat_tool
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

STAT_COM_DEF_FILE=StatComDef.h
OBJ=main.o
all: $(STAT_COM_DEF_FILE) $(TARGET)
$(TARGET):$(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)   
	install  $(TARGET) ../../../bin/
%.o: %.cpp                                         
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 
%.o: %.c                                           
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)  
$(STAT_COM_DEF_FILE):
	@cp ../../comm/stat_mgr/StatComDef.h ./
clean:
	@-rm $(STAT_COM_DEF_FILE) $(OBJ) $(TARGET) -rf


