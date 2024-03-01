GET_PKG_ARCH=$(shell uname -a|grep tlinux)
GET_PKG_ARCH1=$(shell uname -a|grep x86_64)
ifeq ($(GET_PKG_ARCH),)
    ifeq ($(GET_PKG_ARCH1),)
        PKG_ARCH=SuSe_32
    else
        PKG_ARCH=SuSe_64
    endif
else
    PKG_ARCH=tlinux_64
endif

BINDIR=../../../bin/
FILENAME=segvtool.sh

all:
	@echo segvtool copy!!!
ifeq ($(PKG_ARCH), SuSe_32)
	@cp segvtool_32.sh $(BINDIR)${FILENAME}
else
	@cp segvtool_64.sh $(BINDIR)${FILENAME}
endif

clean:
	@echo segvtool clean!!!
	@rm ../../../bin/segvtool.sh -rf
