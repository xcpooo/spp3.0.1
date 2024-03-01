#ifndef _SPP_PROXY_DEFAULT_H_
#define _SPP_PROXY_DEFAULT_H_
#include "../comm/frame.h"
#include "../comm/serverbase.h"
#include "../comm/tbase/tshmcommu.h"
#include "../comm/tbase/tsockcommu/tsockcommu.h"
#include "../comm/benchapiplus.h"
#include "../comm/singleton.h"
#include "../comm_def.h"
#include <vector>
#include <set>
#include <dlfcn.h>
#include <map>
#define IPLIMIT_DISABLE		  0x0
#define IPLIMIT_WHITE_LIST    0x1
#define IPLIMIT_BLACK_LIST    0x2
#define NEED_ROUT_FLAG        -10000

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace std;

namespace spp
{
    namespace proxy
    {
        class CDefaultProxy : public CServerBase, public CFrame
        {
        public:
            CDefaultProxy();
            ~CDefaultProxy();

            //实际运行函数
            void realrun(int argc, char* argv[]);
            //服务容器类型
            int servertype() {
                return SERVER_TYPE_PROXY;
            }
            //初始化配置
            int initconf(bool reload = false);

            //一些回调函数
            static int ator_recvdata(unsigned flow, void* arg1, void* arg2);	//必要
            static int ctor_recvdata(unsigned flow, void* arg1, void* arg2);	//必要
            static int ator_overload(unsigned flow, void* arg1, void* arg2);	//非必要
            static int ator_connected(unsigned flow, void* arg1, void* arg2);   //非必要
            //static int ator_recvdone(unsigned flow, void* arg1, void* arg2);	//非必要
            //static int ator_timeout(unsigned flow, void* arg1, void* arg2);	//非必要
            //static int ctor_recvdone(unsigned flow, void* arg1, void* arg2);	//非必要
            static int ator_senderror(unsigned flow, void* arg1, void* arg2);	//非必要
            static int ator_recvdata_v2(unsigned flow, void* arg1, void* arg2);	//必要
            static int ator_disconnect(unsigned flow, void* arg1, void* arg2); //非必要


            //接受者
            CTCommu* ator_;
            //连接者，共享内存队列
            map<int, CTCommu*> ctor_;
            //ip限制类型
            unsigned char iplimit_;
            //ip集合
            set<unsigned> iptable_;

            spp_handle_process_t local_handle;

            // commu local flag
            int    commu_local_;
        };
    }
}
#endif
