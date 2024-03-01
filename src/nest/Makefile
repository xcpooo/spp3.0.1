### NEST ROOT MAKE
AGENT_VERSION=`cat ./agent/agent_version.h | grep NEST_AGENT_VERSION | grep -v grep | sed 's/^.*:\s*\([0-9]*.*_[0-9]*\).*$$/\1/g'`

all:
	tar -zxvf ./exlib/pb.tgz -C ./exlib
	make all -C ./comm/
	make all -C ./agent/
	make all -C ./proxy/
	make all -C ./worker/
	cp ../../bin/lib/ ./dist/ -rf
	cp ./agent/nest_agent ./dist/
	cp ./agent/nest_clone ./dist/
	cp ./agent/nest_tool  ./dist/
	cp ./agent/nest_start_tool ./dist/
	cp ./proxy/nest_proxy ./dist/ 
	cp ./worker/nest_worker ./dist/
	cp ./proxy/nest_proxy ../../bin
	cp ./worker/nest_worker ../../bin
	cp ./agent/nest_tool ../../bin
	cp ./agent/nest_start_tool ../../bin
	cp ./script/service_restart.sh ../../bin

clean:
	rm ./exlib/pb -rf
	rm ../../bin/nest_proxy -rf
	rm ../../bin/nest_worker -rf
	rm ./dist/* -rf
	make clean -C ./comm/
	make clean -C ./agent/
	make clean -C ./proxy/
	make clean -C ./worker/

dist: all
	mkdir -p ./nest_agent_release
	mkdir -p ./nest_agent_release/bin
	mkdir -p ./nest_agent_release/conf
	mkdir -p ./nest_agent_release/tool
	cp ./dist/lib ./nest_agent_release/bin -rf
	cp ./dist/nest_agent ./nest_agent_release/bin
	cp ./dist/nest_clone ./nest_agent_release/bin
	cp ./agent/nest_tool ./nest_agent_release/tool
	cp ./agent/agent_exec_tool ./nest_agent_release/tool
	cp ./agent/nest_start_tool ./nest_agent_release/tool
	cp ./script/nest_agent.xml ./nest_agent_release/conf
	cp ./script/nest_agent.sh ./nest_agent_release/tool
	tar czf ./dist/nest_agent_$(AGENT_VERSION).tar.gz ./nest_agent_release 
	rm ./nest_agent_release -rf 

all32:
	make -e ARCH=32
