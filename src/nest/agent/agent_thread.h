/**
 * @brief Nest agent thread 
 */
 
#include <stdint.h>
#include <vector>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"
#include "nest_thread.h"
#include "agent_msg.h"
#include "sys_info.h"


#ifndef __NEST_AGENT_THREAD_H_
#define __NEST_AGENT_THREAD_H_

using std::map;
using std::vector;

namespace nest
{

    /**
     * @brief 负载扫描线程
     */
    class CThreadSysload : public CNestThread
    {
    public:

        // 线程执行接口函数
        virtual void Run();

        // 资源清理回收接口
        virtual ~CThreadSysload();

        // 提取更新进程信息
        void ExtractProc(vector<uint32_t>& pids);

        // 清理进程列表
        void CleanProcMap(proc_map_t& proc_map);

        // 更新采集信息
        void UpdateLoad(uint32_t time_interval);

        // 打包负载上报消息
        int32_t PackReportPkg(void* buff, uint32_t buff_len);

        // 负载采集上报负载解包
        int32_t UnPackReportRsp(void* buff, uint32_t buff_len);

        // 同步负载上报处理
        int32_t ReportSendRecv();

    protected:

        CSysInfo           _sys_info;           // 系统负载信息
        proc_map_t         _proc_map;           // 进程负载信息
    };

    /**
     * @brief 进程管理的公共部分, 抽象基类
     */
    class CThreadProcBase 
    {
    public:

        /**
         * @brief 修改子进程的用户名
         * @return =0 成功 <0 失败
         */
        static int32_t ChgUser(const char *user);

        /**
         * @brief 检查path是否存在，如果不存在则创建
         */
        static bool CheckAndMkdir(char *path);
        
        
        /**
         * @brief 递归创建目录
         * @return =0 成功 <0 失败
         */
        static int32_t MkdirRecursive(char *path, size_t len);

        /**
         * @brief 检查并重映射/data目录到私有的path
         */
        static bool MountPriPath(string& package);

        
        /**
         * @brief clone执行进程初始化的入口函数
         */
        static int32_t CloneEntry(void* args);

        /**
         * @brief 执行隔离目录的进程clone操作
         * @return >0 子进程启动OK, 返回PID, 否则失败
         */
        pid_t DoFork(void* args);

    
    public:

        // 构造函数
        CThreadProcBase(){};   

        // 析构函数
        virtual ~CThreadProcBase(){};

        // 获取spp包基准目录
        char* GetBinBasePath();

        // 启动单个进程的执行函数
        int32_t StartProc(const nest_proc_base& conf, nest_proc_base& proc);

        // 停止单个进程运行
        void DelProc(const nest_proc_base& proc);

        // 等待进程启动执行到创建unix端口
        void CheckWaitStart(uint32_t pid, uint32_t wait_ms);
        
        // 等待进程停止执行
        void CheckWaitStop(uint32_t pid, uint32_t wait_ms);

        // 封装进程启动报文
        int32_t PackInitReq(void* buff, uint32_t buff_len, nest_proc_base& proc);

        // 解析进程启动应答
        int32_t UnPackInitRsp(void* buff, uint32_t buff_len, nest_proc_base& proc, uint32_t& worker_type);

        // 驱动进程初始化执行
        int32_t InitSendRecv(nest_proc_base& proc, uint32_t& worker_type);

        // 设置业务名称, 一次仅处理一个业务
        void SetServiceName(const string& service_name) {
            _service_name = service_name;
        };
        
        // 设置路径名称, 一次仅处理一个路径
        void SetPackageName(const string& pkg_name) {
            _package_name = pkg_name;
        };

    protected:

        string          _service_name;          // 业务名
        string          _package_name;          // 路径名

    };
    

    /**
     * @brief 进程创建工作线程部分
     */
    class CThreadAddProc : public CNestThread, public CThreadProcBase
    {
    public:

        // 线程执行接口函数
        virtual void Run();

        // 资源清理回收接口
        virtual ~CThreadAddProc() {};

        // 构造函数
        CThreadAddProc() {
            _restart_flag = false;
        };

        // 新建进程处理, 需要应答报文
        void RunStart();

        // 重启进程接口, 无需应答消息
        void RunRestart();

        // 新建任务, 记录上下文信息
        void SetCtx(TNestBlob* blob, TMsgAddProcReq* req);

        // 重启任务, 记录上下文信息
        void SetRestartCtx(nest_sched_add_proc_req* req);

    protected:

        TNestBlob                 _blob;                // 上下文消息块
        nest_msg_head             _msg_head;            // 通用消息头
        nest_sched_add_proc_req   _req_body;            // 通用进程添加请求接口
        nest_sched_add_proc_rsp   _rsp_body;            // 通用的进程添加应答
        bool                      _restart_flag;        // 是否处理重启请求    
    };

    
    /**
     * @brief 进程删除工作线程部分
     */
    class CThreadDelProc : public CNestThread, public CThreadProcBase
    {
    public:

        // 线程执行接口函数
        virtual void Run();
        
        // 资源清理回收接口
        virtual ~CThreadDelProc() {};

        // 删除任务, 记录上下文信息
        void SetCtx(TNestBlob* blob, TMsgDelProcReq* req);

        // 执行进程删除实现
        int32_t DeleteProcess(const nest_proc_base& proc);

    protected:

        TNestBlob                 _blob;                // 上下文信息
        nest_msg_head             _msg_head;            // 消息头
        nest_sched_del_proc_req   _req_body;            // 请求消息
        nest_sched_del_proc_rsp   _rsp_body;            // 应答消息
    };

    
    /**
     * @brief 进程重启消息工作线程部分
     */
    class CThreadRestartProc : public CNestThread, public CThreadProcBase
    {
    public:

        // 线程执行接口函数
        virtual void Run();

        // 资源清理回收接口
        virtual ~CThreadRestartProc() {};

        // 构造函数
        CThreadRestartProc() {};

        // 新建任务, 记录上下文信息
        void SetCtx(TNestBlob* blob, TMsgRestartProcReq* req);

    protected:

        TNestBlob                   _blob;                // 上下文消息块
        nest_msg_head               _msg_head;            // 通用消息头
        nest_sched_restart_proc_req _req_body;            // 通用进程添加请求接口
        nest_sched_restart_proc_rsp _rsp_body;            // 通用的进程添加应答
    };

}


#endif

