#ifndef _SPP_WORKER_DEFAULT_H_
#define _SPP_WORKER_DEFAULT_H_
#include "../comm/frame.h"
#include "../comm/serverbase.h"
#include "../comm/monitor.h"
#include "../comm/tbase/tshmcommu.h"
#include "../comm/tbase/tsockcommu/tsockcommu.h"

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;

//for segv tool
extern CServerBase* g_worker;

namespace spp
{
    namespace worker
	{
        class CDefaultWorker : public CServerBase, public CFrame
        {
        public:
            CDefaultWorker();
            ~CDefaultWorker();

            // 获取是否启用微线程
            bool get_mt_flag();

            // 微线程切换函数
            void handle_switch(bool block);

            //实际运行函数
            void realrun(int argc, char* argv[]);
            //服务容器类型
            int servertype() {
                return SERVER_TYPE_WORKER;
            }

            // 注册spp框架信号处理函数
            void assign_signal(int signo);
            
            //初始化配置
            int initconf(bool reload = false);

            static void shm_delay_stat(int64_t time_delay)
            {
                if(time_delay <= 1)
                {
                    MONITOR(MONITOR_WORKER_RECV_DELAY_1);
                }
                else if(time_delay <= 10)
                {
                    MONITOR(MONITOR_WORKER_RECV_DELAY_10);
                }
                else if(time_delay <= 50)
                {
                    MONITOR(MONITOR_WORKER_RECV_DELAY_50);
                }
                else if(time_delay <= 100)
                {
                    MONITOR(MONITOR_WORKER_RECV_DELAY_100);
                }
                else
                {
                    MONITOR(MONITOR_WORKER_RECV_DELAY_XXX);
                }
            }

            //一些回调函数
            static int ator_recvdata(unsigned flow, void* arg1, void* arg2);	//必要
            static int ator_recvdata_v2(unsigned flow, void* arg1, void* arg2);	//必要
            static int ator_senddata(unsigned flow, void* arg1, void* arg2);	//非必要
            static int ator_overload(unsigned flow, void* arg1, void* arg2);	//非必要
            static int ator_senderror(unsigned flow, void* arg1, void* arg2);	//必要

            //接受者
            CTCommu* ator_;
            
			inline int get_TOS(){return TOS_;}
		private:
            unsigned msg_timeout_;
			int TOS_;
            int commu_local_;   // unix domain socket commu
            int notify_fd_;     // socket commu need notify mircro thread
	
        };
    }
}
#endif
