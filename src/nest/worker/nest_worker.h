/**
 * @brief WORKER NEST 
 * @info  最大限度兼容支持spp worker
 */

#ifndef _NEST_WORKER_DEFAULT_H_
#define _NEST_WORKER_DEFAULT_H_
#include "frame.h"
#include "serverbase.h"
#include "tshmcommu.h"
#include "tsockcommu.h"
#include "proc_comm.h"

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;

using namespace nest;

extern CServerBase* g_worker;

namespace nest
{

    class CWorkerCtrl;       // worker 控制代理类
    class CNestWorker;       // worker 框架类

    /**
     * @brief 公共进程对象定义
     */
    class CWorkerCtrl : public CProcCtrl
    {
    public:

        ///< 构造与析构函数
        CWorkerCtrl() {};
        virtual ~CWorkerCtrl() {};

        ///< 虚接口, 控制命令管理接口实现
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< 控制消息-初始化进程消息处理
        int32_t RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< 关联全局进程对象
        void SetServerBase(CNestWorker* base) {
            _server_base = base;    
        };

    protected:

        CNestWorker*           _server_base;     // 服务器类指针
    };


    /**
     * @brief 框架类定义, 私有实现
     */
    class CNestWorker : public CServerBase, public CFrame
    {
    public:

        ///< 构造与析构函数
        CNestWorker();
        ~CNestWorker();

        ///< 鸟巢特殊由fork产生, 特殊初始化
        virtual void startup(bool bg_run = true);
        
        ///< 进程真实执行函数
        void realrun(int32_t argc, char* argv[]);
        
        ///< 默认的进程类型
        int32_t servertype() {
            return SERVER_TYPE_WORKER;
        }

        ///< 注册spp信号处理函数
        void assign_signal(int signo);
        
        ///< 配置初始化
        int32_t initconf(bool reload = false);

        ///< 回调函数处理
        static int32_t ator_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_recvdata_v2(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_senddata(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_overload(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_senderror(uint32_t flow, void* arg1, void* arg2);	

        ///< 获取tos接口函数
		inline int32_t get_TOS(){return TOS_;}

        ///< 执行运行中的例行任务
        void handle_routine();
        
        ///< 执行业务流程
        void handle_service();
        
        ///< 执行退出前的任务
        void handle_before_quit();
        
        ///< 初始化控制接口, 启动控制侦听
        bool init_ctrl_channel();

        ///< 检查是否启动微线程
        bool micro_thread_enable();

        ///< 切换到微线程的调度
        void micro_thread_switch(bool block);

    public:

        CTCommu*         ator_;                 // 前端接收器句柄
        CWorkerCtrl      ctrl_;                 // 控制块对象
        uint32_t         initiated_;            // 是否初始化, 0 未初始化
		
	private:
	
        uint32_t         msg_timeout_;          // 超时时间
		int32_t          TOS_;                  // TOS相关设置
		int32_t          notify_fd_;            // socket通知句柄 
		bool             mt_flag_;              // 是否使用微线程
    };
}

#endif

