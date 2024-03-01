BUILD_DIR=../../comm

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

all:
	cp ../../comm/tbase/tstat.h .
	cp ../../comm/tbase/tstat_policy.h .
	cp ../../comm/tbase/tstat.cpp .
	g++ -g stat.cpp -I../../comm/tbase/ -c $(CFLAGS)
	g++ -g tstat.cpp -I../../comm/tbase/ -c $(CFLAGS)
	g++ *.o -o stat_tool $(CFLAGS)
	install stat_tool  ../../../bin/
clean:
	@rm -rf *.o stat_tool
	@rm -rf tstat*

