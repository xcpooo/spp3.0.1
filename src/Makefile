all:
	-mkdir ../bin/
	-mkdir ../bin/lib/
	-mkdir ../moni/
	-mkdir ../stat/
	-mkdir ../log/
	find . -name "*.sh" | xargs chmod a+x 
	cp ./script/*.sh ../bin/
	make all -C ./comm/
	make all -C ./async/
	make all -C ./async_frame/
	make all -C ./sync_frame/
	make all -C ./external/segv
	make all -C ./nest/
	make all -C ./proxy/
	make all -C ./controller/
	make all -C ./worker/
	make all -C ./tool/segv_tool/
	make all -C ./tool/shm_key_tool/
	make all -C ./tool/shm_read_tool/
	make all -C ./tool/stat_tool/
	make all -C ./tool/yaml_tool/
	make all -C ./tool/dc_tool/
	make all -C ./tool/cost_stat_tool/
	make copy -C ./module/
	
clean:
	make clean -C ./comm/
	make clean -C ./async/
	make clean -C ./async_frame/
	make clean -C ./sync_frame/
	make clean -C ./external/segv
	make clean -C ./proxy/
	make clean -C ./controller/
	make clean -C ./worker/
	make clean -C ./module/
	make clean -C ./tool/segv_tool/
	make clean -C ./tool/shm_key_tool/
	make clean -C ./tool/shm_read_tool/
	make clean -C ./tool/stat_tool/
	make clean -C ./tool/yaml_tool/
	make clean -C ./tool/dc_tool/
	make clean -C ./tool/cost_stat_tool/
	make clean -C ./nest/
	-rm -rf ../bin/
	-rm -rf ../moni/
	-rm -rf ../stat/
	-rm -rf ../log/
	-rm -rf ./publish/spp*

all32:
	make -e ARCH=32
