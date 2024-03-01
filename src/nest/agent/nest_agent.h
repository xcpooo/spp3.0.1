/**
 * @brief Nest agent process 
 */
#ifndef _NEST_AGENT_PROCESS_H_
#define _NEST_AGENT_PROCESS_H_

#include <stdint.h>
#include <set>
#include <map>
#include "frame.h"
#include "serverbase.h"
#include "comm_def.h"
#include "global.h"
#include "timerlist.h"
#include "nest_channel.h"
#include "nest_proto.h"
#include "agent_process.h"
#include "nest_thread.h"
#include "agent_msg.h"
#include "nest_timer.h"
#include "agent_cgroups.h"

using namespace tbase::tlog;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace spp::comm;
using namespace std;
using namespace nest;


namespace nest
{



    /**
     * @brief  Agent执行任务的上下文, 记录请求来源等信息
     */
    class CTaskCtx : public CTimerObject
    {
    public:

        ///<  构造函数
        CTaskCtx() {
            _taskid = 0;
        };

        ///<  析构函数
        virtual ~CTaskCtx() {
            DisableTimer();
        };

        ///<  超时处理函数
        virtual void TimerNotify();

        ///<  启动定时任务
        void EnableTimer(uint32_t wait_time);

        ///<  设置上下文信息
        void SetBlobInfo(TNestBlob* blob) {
            _blob = *blob;
        };

        ///<  获取上下文信息
        TNestBlob& GetBlobInfo() {
            return _blob;
        };

        ///<  设置任务id信息
        void SetTaskId(uint32_t taskid) {
            _taskid = taskid;
        };

        ///<  获取任务id信息
        uint32_t GetTaskId() {
            return _taskid;
        };

    protected:

        uint32_t                  _taskid;          ///< 任务id, 全局唯一
        TNestBlob                 _blob;            ///< 上下文信息
    };
    typedef std::map<uint32_t, CTaskCtx*>  NestTaskMap;


    /**
     * @brief Agent执行代理类
     */
    class CAgentProxy
    {
    public:
    
        ///< 单例类访问函数
        static CAgentProxy* Instance();

        ///< 全局的销毁接口
        static void Destroy();

        ///< 析构清理函数
        virtual ~CAgentProxy();

        ///< 实际初始化函数
        int32_t Init();

        ///< 设置调度的连接信息
        int32_t DumpSetInfo();

        ///< 读取调度的连接信息
        void LoadSetInfo();

        ///< 更新调度的set信息
        int32_t UpdateSetInfo(uint32_t setid, TNestAddrArray& servers);

        ///< 清理调度的set信息
        int32_t ClearSetInfo();

        ///< 提取地址 转换地址结构
        void ExtractServerList(TNestAddrArray& svr_list);
        
        ///< 消息分发处理函数
        int32_t DispatchMsg(void* args);

        ///< 消息处理句柄注册与查询函数
        void RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle);
        TNestMsgHandler* GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd);

        ///< 应答打包与发送接口
        int32_t SendRspMsg(TNestBlob* blob, string& head, string& body); 
        int32_t SendToAgent(string& head, string& body); 

        ///< 任务管理接口函数, 添加任务项
        void AddTaskCtx(uint32_t taskid, CTaskCtx* ctx);

        ///< 任务管理接口函数, 删除任务项
        void DelTaskCtx(uint32_t taskid);

        ///< 任务管理接口函数, 查找任务项
        CTaskCtx* FindTaskCtx(uint32_t taskid);

        ///< 全局的分派taskid, 本地唯一
        uint32_t GetTaskSeq();

        ///< stat上报组合函数封装
        static void ProcStatSet(nest_proc_base& base, StatUnit& stat, LoadUnit& load, nest_proc_stat* report);

        /**
         * @brief 定时任务回调函数定义
         */
        static int32_t KeepConnectionTimer(void* args);
        static int32_t HeartbeatTimer(void* args);
        static int32_t LoadReportTimer(void* args);
        static int32_t DcToolTimer(void* args);
        static int32_t TaskCheckTimer(void* args);

        ///< 获取setid
        uint32_t GetSetId() {
            return _set_id;
        };

        ///< 获取cgroups管理句柄
        CAgentCGroups& GetCGroupHandle() {
            return _cgroups;
        };



    private:

        ///< 构造函数
        CAgentProxy();
        
        static CAgentProxy*                 _instance;    // 单例实例
        
        uint32_t                            _set_id;      // set信息
        std::vector<string>                 _set_servers; // set服务器信息
        NestTaskMap                         _task_map;    // 任务列表
        MsgDispMap                          _msg_map;     // 消息注册管理
        CAgentCGroups                       _cgroups;     // cgroups管理

    public:
        static string                       _cluster_type;// 集群类型 spp or sf2 

    };

    /**
     * @brief 鸟巢代理框架定义
     */
    class NestAgent : public CServerBase, public CFrame
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        NestAgent():_clone_proc_pid(-1) {};
        ~NestAgent() {};

        /**
         * @brief 业务启动函数, 环境初始化
         */
        virtual void run(int argc, char* argv[]);

        /**
         * @brief 业务运行函数, 主循环
         */
        virtual void realrun(int argc, char* argv[]);

        /**
         * @brief 配置初始化函数
         */
        int32_t initconf(int argc, char* argv[]);
	private:
		///< 启动 clone 进程
		int32_t startCloneProc(int argc, char* argv[]);

		///< 停止 clone 进程
		int32_t stopCloneProc();
		

	private:
		pid_t								_clone_proc_pid; // clone 进程的pid
    };

}




#endif


