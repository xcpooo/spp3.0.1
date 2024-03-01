/**
 * @brief PROXY NEST 
 * @info  最大限度兼容支持spp proxy
 */


#ifndef _NEST_PROXY_DEFAULT_H_
#define _NEST_PROXY_DEFAULT_H_

#include "stdint.h"
#include "frame.h"
#include "serverbase.h"
#include "tshmcommu.h"
#include "tsockcommu.h"
#include "benchapiplus.h"
#include "singleton.h"
#include "comm_def.h"
#include <vector>
#include <set>
#include <dlfcn.h>
#include <map>
#include "proc_comm.h"

#define IPLIMIT_DISABLE		  0x0
#define IPLIMIT_WHITE_LIST    0x1
#define IPLIMIT_BLACK_LIST    0x2
#define NEED_ROUT_FLAG        -10000

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace std;
using namespace nest;

namespace nest
{
    class CProxyCtrl;       // proxy 控制代理类
    class CNestProxy;       // proxy 框架类

    /**
     * @brief 公共进程对象定义
     */
    class CProxyCtrl : public CProcCtrl
    {
    public:

        ///< 构造与析构函数
        CProxyCtrl() {};
        virtual ~CProxyCtrl() {};

        ///< 虚接口, 控制命令管理接口实现
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< 控制消息-初始化进程消息处理
        int32_t RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< 关联全局进程对象
        void SetServerBase(CNestProxy* base) {
            _server_base = base;    
        };

    protected:

        CNestProxy*           _server_base;     // 服务器类指针
    };


    /**
     * @brief 框架类定义, 私有实现
     */
    class CNestProxy : public CServerBase, public CFrame
    {
    public:

        ///< 构造与析构函数
        CNestProxy();
        ~CNestProxy();

        ///< 鸟巢特殊由fork产生, 特殊初始化
        virtual void startup(bool bg_run = true);

        ///< 进程真实执行函数
        void realrun(int argc, char* argv[]);

        ///< 默认的进程类型
        int32_t servertype() {
            return SERVER_TYPE_PROXY;
        }
        
        ///< 配置初始化
        int32_t initconf(bool reload = false);

        ///< 回调函数处理
        static int32_t ator_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ctor_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_overload(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_connected(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_senderror(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_recvdata_v2(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_disconnect(uint32_t flow, void* arg1, void* arg2);

        
        ///< 执行运行中的例行任务
        void handle_routine();
        
        ///< 执行业务流程
        void handle_service();

        ///< 执行退出前的任务
        void handle_before_quit();

        ///< 接收后端回包
        int32_t recv_ctor_rsp();

        ///< 初始化控制接口, 启动控制侦听
        bool init_ctrl_channel();

    public:
        
        CTCommu*                    ator_;              ///< 前端接收器
        map<int32_t, CTCommu*>      ctor_;              ///< 后端连接器
        uint8_t                     iplimit_;           ///< ip限制类型 禁用-白名单-黑名单
        set<uint32_t>               iptable_;           ///< ip集合
        uint32_t                    initiated_;         ///< 是否初始化, 0 未初始化

        spp_handle_process_t        local_handle;
        
        CProxyCtrl                  ctrl_;                         
    };



    
}
#endif
