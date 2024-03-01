/**
 * @brief Nest agent clone
 *  fork proxy/worker
 */
#ifndef _NEST_AGENT_CLONE_H_
#define _NEST_AGENT_CLONE_H_

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
#include "agent_msg_incl.h"
#include "nest_timer.h"
#include "agent_cgroups.h"
#include "agent_net_dispatch.h"

using namespace tbase::tlog;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace spp::comm;
using namespace std;
using namespace nest;


namespace nest
{

	class TMsgProcFork:public TNestMsgHandler
	{
	public:
        // 实际的消息处理接口函数, 继承实现
        int32_t Handle(void* args);	
	private:
		static int32_t CloneEntry(void* args);
		pid_t DoFork(void* args);
	protected:
        nest_msg_head             _msg_head;        // 同步处理消息头信息
        nest_sched_proc_fork_req  _req_body;        // 请求包结构
        nest_sched_proc_fork_rsp  _rsp_body;        // 应答包结构	
	};


    /**
     * @brief Agent执行代理类
     */
    class CCloneProxy:public TMsgDispatch
    {
    public:

        ///< 单例类访问函数
        static CCloneProxy* Instance();

        ///< 全局的销毁接口
        static void Destroy();

        ///< 析构清理函数
        virtual ~CCloneProxy();

        ///< 实际初始化函数
        int32_t Init();

        ///< 消息分发处理函数
        int32_t dispatch(void* args);

        ///< 消息处理句柄注册与查询函数
        void RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle);
        TNestMsgHandler* GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd);

        ///< 应答打包与发送接口
        int32_t SendRspMsg(TNestBlob* blob, string& head, string& body);

        ///< 读取调度的连接信息
        void LoadSetInfo();

        ///< 提取地址 转换地址结构
        void ExtractServerList(TNestAddrArray& svr_list);
        ///< 获取cgroups管理句柄
        CAgentCGroups& GetCGroupHandle() {
            return _cgroups;
        };
    private:

        ///< 构造函数
        CCloneProxy();

        static CCloneProxy*                 _instance;    // 单例实例

        uint32_t                            _set_id;      // set信息
        std::vector<string>                 _set_servers; // set服务器信息
        MsgDispMap                          _msg_map;     // 消息注册管理

        CAgentCGroups                       _cgroups;     // cgroups管理
    public:
        static std::string                         _cluster_type;
    };




    /**
     * @brief 蜂巢clone框架定义
     */
    class NestClone : public CServerBase, public CFrame
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        NestClone() {};
        ~NestClone() {};

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

    };

}




#endif



