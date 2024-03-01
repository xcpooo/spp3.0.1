#include "defaultworker.h"
#include "comm_def.h"
#include "segv_api.h"

using namespace spp;
using namespace spp::worker;

#define SEGV_TLINUX_MAIN_END \
	v__sBlankGap_first[0] = v__sBlankGap_first[0];\
	v__sBlankGap_last[0] = v__sBlankGap_last[0];

// Coredump Check Point
// 进入待检测区域时，设置对应的CheckPoint， 退出待检测区域时，恢复为CoreCheckPoint_SppFrame
CoreCheckPoint g_check_point = CoreCheckPoint_SppFrame;

struct sigaction g_sa;
CServerBase* g_worker = NULL;

char* checkpoint2str(CoreCheckPoint p)
{
    if(p == CoreCheckPoint_SppFrame)
        return "spp frame";
    else if(p == CoreCheckPoint_HandleProcess)
        return "spp_handle_process";
    else if(p == CoreCheckPoint_HandleFini)
        return "spp_handle_fini";
    else if(p == CoreCheckPoint_HandleInit)
        return "spp_handle_init";
    else
        return "unknown";
}


void sigsegv_handler(int sig)
{
    if (g_worker)
    {
        signal(sig, SIG_DFL);
        //((CDefaultWorker *)g_worker)->flog_.LOG_P_PID(LOG_FATAL,
         //   "[!!!!!] illegal signal %d at %s\n", sig, checkpoint2str(g_check_point));
    }
    return;
}

int main(int argc, char* argv[])
{
    SEGV_DECLARE_FIRST
    g_worker = new CDefaultWorker;
    SEGV_DECLARE_LAST
    
    SEGV_TREAT(NULL, NULL);

    if (g_worker)
    {
        g_worker->run(argc, argv);

        delete g_worker;
    }

    SEGV_TLINUX_MAIN_END

    return 0;
}
