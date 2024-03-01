/*
	add by aoxu
   frame.h is included by spp frame but not by module.
*/

#ifndef _SPP_COMM_FRAME_
#define _SPP_COMM_FRAME_
#include <string.h>
#include "tbase/tlog.h"
#include "tbase/tstat.h"

#define LOG_SCREEN(lvl, fmt, args...)   do{ printf("[%d] ", getpid());printf(fmt, ##args); flog_.LOG_P_PID(lvl, fmt, ##args); }while (0)

using namespace tbase::tlog;
using namespace tbase::tstat;
using namespace std;

namespace spp
{
    namespace comm
    {
		//框架公共类
		class CFrame
        {
        public:
            CTLog flog_;	//框架日志对象
			CTLog monilog_;	//框架监控日志对象
			CTStat fstat_;	//框架统计对象
            int groupid_;
            int realtime_;
        };
    }
}

#endif

