#ifndef _SPP_CTRL_DEFAULT_H_
#define _SPP_CTRL_DEFAULT_H_
#include<string>
#include<vector>
#include <map>
#include "../comm/frame.h"
#include "../comm/serverbase.h"
#include"../comm/comm_def.h"
#include "../comm/tbase/hide_private_tp.h"
#include "../comm/tbase/tcommu.h"
#include "../comm/tbase/tshmcommu.h"
#include "../comm/tbase/hide_private_tp.h"
#include "StatComDef.h"
using namespace::std;
using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;

namespace spp
{
    namespace ctrl
    {
        class CDefaultCtrl : public CServerBase, public CFrame
        {
        public:
            CDefaultCtrl();
            ~CDefaultCtrl();

            //实际运行函数
            void realrun(int argc, char* argv[]);
            //服务容器类型
            int servertype() {
                return SERVER_TYPE_CTRL;
            }

            void dump_pid();
            void check_report_tool();
            void check_ctrl_running();

            void clear_stat_file();

            //初始化配置
            int initconf(bool reload = false);
            int reloadconf();
            int initmap(bool reload);

        protected:
            //监控srv
            CTProcMonSrv monsrv_;
            // report tool
            std::string report_tool;
            // 输出pid文件fd
            int pidfile_fd_;
            // 退出时，执行的指令
            vector<string> cmd_on_exit_;

        };
    }
}
#endif
