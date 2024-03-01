/**
 * @brief Nest agent net manager 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"


#ifndef __NEST_AGENT_NET_DISPATCH_H_
#define __NEST_AGENT_NET_DISPATCH_H_

using std::map;
using std::vector;

namespace nest
{
	class TMsgDispatch
	{
	public:
        virtual ~TMsgDispatch(){}
		virtual int32_t dispatch(void* msg) { return -1; };	
	};

	typedef std::map<uint32_t, TNestListen *>  TNestListenMap;
    typedef std::vector<TNestListen*>          TNestListenList;

  
    /** 
     * @brief 网络远程交互连接器的全局管理
     */
    class  TAgentNetDispatch : public TNestDispatch
    {
    public:

        ///< 单例类访问函数
        static TAgentNetDispatch* Instance();

        ///< 全局的销毁接口
        static void Destroy();
        

        ///< 析构清理函数
        virtual ~TAgentNetDispatch();

		int32_t AddListen(const TNestAddress& addr, bool tcp = true);
		int32_t SetMsgDispatch(TMsgDispatch *msg_handler);

        ///< 查询动态连接信息
        TNestChannel* FindChannel(uint32_t fd);  


        /**
         * @brief 分发处理函数
         */
        virtual int32_t DoRecv(TNestBlob* channel);
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        
        /**
         * @brief 执行消息收发, 启动服务运行
         */
        void RunService();

    private:

        // 单例构造函数
        TAgentNetDispatch();
        
        static TAgentNetDispatch*        _instance;       // 单例对象

        TNestChannelMap             _channel_map;    // 连接管理map
		TNestListenMap             _listen_map;     // 本地监听列表
        

		TMsgDispatch*				_msg_dispatch;     // 消息处理对象
    };


}

#endif


