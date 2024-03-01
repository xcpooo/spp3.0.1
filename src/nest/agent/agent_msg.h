/**
 * @brief Nest agent msg 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"
#include "agent_msg_incl.h"

#ifndef __NEST_MSG_H_
#define __NEST_MSG_H_

using std::map;
using std::vector;

namespace nest
{

    /**
     * @brief 节点控制消息处理实现
     */
    class TMsgNodeSet :     public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 提取agent server地址信息
        void ExtractServers(TNestAddrArray& servers);

        // 获取当前选中的server信息
        char* GetCurrServer();

    protected:
    
        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_sched_node_init_req  _req_body;        // 请求包结构
        nest_sched_node_init_rsp  _rsp_body;        // 应答包结构
    };

    /**
     * @brief 节点删除消息处理实现
     */
    class TMsgNodeDel :     public TNestMsgHandler
    {
    public:
    
        // 消息处理接口函数
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head             _msg_head;         // 同步处理消息头信息
        nest_sched_node_term_req  _req_body;         // 请求包结构
    };

    /**
     * @brief 进程创建消息处理实现
     */
    class TMsgAddProcReq :     public TNestMsgHandler
    {
    public:

        // 防止重复创建的检查函数
        int32_t CheckProcExist(string& errmsg);

        // 本地分配proxy的端口, 临时逻辑处理
        int32_t AllocProxyPort(string& errmsg);
        
        // 如果调度中心分配了非0的port,尝试使用, 否则hash分配
        int32_t CheckAllocPort(nest_proc_base* proc_conf, string& errmsg);
    
        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 获取消息头接口
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // 获取消息请求包体
        nest_sched_add_proc_req& GetMsgReqBody() {
            return _req_body;
        };
        

    protected:

        nest_msg_head             _msg_head;         // 同步处理消息头信息
        nest_sched_add_proc_req   _req_body;         // 请求包结构
        nest_sched_add_proc_rsp   _rsp_body;         // 应答包结构
    };
    

    /**
     * @brief 进程创建消息应答处理
     */
    class TMsgAddProcRsp :      public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 处理成功更新进程信息
        void UpdateProc();

        // 处理失败,删除进程
        void DeleteProc();

        // 失败时, 回滚进程占用的端口信息
        void FreeProxyPort();

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_sched_add_proc_rsp   _rsp_body;        // 请求包结构
    };
    

    /**
     * @brief 进程删除消息处理实现
     */
    class TMsgDelProcReq :     public TNestMsgHandler
    {
    public:
    
        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 进程删除接口时, 保护防止删除中拉起进程
        void SetProcDeleteFlag();

        // 获取消息头接口
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // 获取消息请求包体
        nest_sched_del_proc_req& GetMsgReqBody() {
            return _req_body;
        };

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_sched_del_proc_req   _req_body;        // 请求包结构
        nest_sched_del_proc_rsp   _rsp_body;        // 应答包结构
    };

    /**
     * @brief 进程删除消息应答处理
     */
    class TMsgDelProcRsp :      public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 处理成功更新进程信息
        void DeleteProc();

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_sched_del_proc_rsp   _rsp_body;        // 请求包结构
    };

    /**
     * @brief 获取业务信息
     */
    class TMsgSrvInfoReq :       public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head                 _msg_head;         // 同步处理消息头信息
        nest_sched_service_info_req   _req_body;         // 请求包结构
        nest_sched_service_info_rsp   _rsp_body;         // 应答包结构
    };


    /**
     * @brief 进程重启消息处理实现
     */
    class TMsgRestartProcReq :     public TNestMsgHandler
    {
    public:

        // 检查并设置正确的pid信息
        int32_t CheckProcPids(string& errmsg);

        // 按业务名获取proc列表, 无进程返回小于0
        int32_t GetServiceProc(string& errmsg);

        // 检查输入信息的有效性
        int32_t CheckRestart(string& errmsg);
    
        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 获取消息头接口
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // 获取消息请求包体
        nest_sched_restart_proc_req& GetMsgReqBody() {
            return _req_body;
        };

    protected:

        nest_msg_head                 _msg_head;         // 同步处理消息头信息
        nest_sched_restart_proc_req   _req_body;         // 请求包结构
        nest_sched_restart_proc_rsp   _rsp_body;         // 应答包结构
    };
    

    /**
     * @brief 进程创建消息应答处理
     */
    class TMsgRestartProcRsp :      public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head                 _msg_head;        // 同步处理消息头信息
        nest_sched_restart_proc_rsp   _rsp_body;        // 请求包结构
    };

    /**
     * @brief 内部负载上报接口
     */
    class TMsgSysLoadReq :      public TNestMsgHandler
    {
    public:
    
        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 更新系统负载信息到进程管理结构
        int32_t UpdateSysLoad();

        // 设置活跃的pid列表到应答报文
        void SetRspPidList();

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_agent_sysload_req    _req_body;        // 请求包结构
        nest_agent_sysload_rsp    _rsp_body;        // 应答包结构
    };


    /**
     * @brief 内部心跳上报接口
     */
    class TMsgProcHeartReq :      public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 更新进程的心跳时间
        void UpdateHeartbeat();
        
        // 处理心跳发现进程的逻辑
        void AddProc();

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_proc_heartbeat_req   _req_body;        // 请求包结构
    };

    
    /**
     * @brief 进程负载上报接口
     */
    class TMsgProcLoadReq :      public TNestMsgHandler
    {
    public:

        // 消息处理接口函数
        virtual int32_t Handle(void* args);

        // 更新进程的负载信息
        int32_t UpdateProcLoad();

    protected:

        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_proc_stat_report_req _req_body;        // 请求包结构
    };
    
}

#endif


