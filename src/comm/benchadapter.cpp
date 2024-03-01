#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <dlfcn.h>
#include "tbase/tlog.h"
#include "global.h"
#include "singleton.h"
#include "benchadapter.h"
#include "benchapiplus.h"
using namespace spp::global;
using namespace spp::singleton;
using namespace tbase::tlog;

/////////////////////////////////////////////////////////////////////////////////
//so library
////////////////////////////////////////////////////////////////////////////////

#define PROCTECT_AREA_SIZE (512*1024)

spp_dll_func_t sppdll = {NULL};

int load_bench_adapter(const char* file, bool isGlobal)
{
    if (sppdll.handle != NULL)
    {
        dlclose(sppdll.handle);
    }

    memset(&sppdll, 0x0, sizeof(spp_dll_func_t));

    int flag = isGlobal ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;

    /* 提早发现部分内存写乱现象 */
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    void* handle = dlopen(file, flag);
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    if (!handle)
    {
        printf("[ERROR]dlopen %s failed, errmsg:%s\n", file, dlerror());
        exit(-1);
    }

    void* func = dlsym(handle, "spp_handle_init");

    if (func != NULL)
    {
        sppdll.spp_handle_init = (spp_handle_init_t)dlsym(handle, "spp_handle_init");
        sppdll.spp_handle_input = (spp_handle_input_t)dlsym(handle, "spp_handle_input");
        sppdll.spp_handle_route = (spp_handle_route_t)dlsym(handle, "spp_handle_route");
        sppdll.spp_handle_process = (spp_handle_process_t)dlsym(handle, "spp_handle_process");
        sppdll.spp_handle_fini = (spp_handle_fini_t)dlsym(handle, "spp_handle_fini");
        sppdll.spp_handle_close = (spp_handle_close_t)dlsym(handle, "spp_handle_close");
        sppdll.spp_handle_exception = (spp_handle_exception_t)dlsym(handle, "spp_handle_exception");
        sppdll.spp_handle_loop = (spp_handle_loop_t)dlsym(handle, "spp_handle_loop");
        sppdll.spp_mirco_thread = (spp_mirco_thread_t)dlsym(handle, "spp_mirco_thread");
        sppdll.spp_handle_switch = (spp_handle_switch_t)dlsym(handle, "spp_handle_switch");
        sppdll.spp_set_notify = (spp_set_notify_t)dlsym(handle, "spp_set_notify");		
        sppdll.spp_handle_report = (spp_handle_process_t)dlsym(handle, "spp_handle_report");

        if (sppdll.spp_handle_input == NULL)
        {
            printf("[ERROR]spp_handle_input not implemented.\n");
            exit(-1);
        }

        if (sppdll.spp_handle_process == NULL)
        {
            printf("[ERROR]spp_handle_process not implemented.\n");
            exit(-1);
        }

        //assert(sppdll.spp_handle_input != NULL && sppdll.spp_handle_process != NULL);
        sppdll.handle = handle;
        return 0;
    }
    else
    {
        printf("[ERROR]cannot find spp_handle_init in module.\n");
        return -1;
    }
}

int load_report_adapter(const char* file)
{
	if (sppdll.spp_handle_report == NULL)
	{
		if (sppdll.report)
		{
        	printf("[ERROR]close spp_report.so\n");
			dlclose(sppdll.report);
		}
	
		sppdll.report = dlopen(file, RTLD_NOW | RTLD_GLOBAL);
		if (!sppdll.report)
	        printf("[WARRING]open spp_report.so %p(%s)\n", sppdll.report, dlerror());
		
		if (sppdll.report)
		{
			sppdll.spp_handle_report = (spp_handle_process_t)dlsym(sppdll.report, "spp_handle_report");
		}
	}

	
	if (sppdll.spp_handle_report == NULL)
	{
	     printf("[WARRING]spp_handle_report not implemented.\n");
	}

	return 0;
}

