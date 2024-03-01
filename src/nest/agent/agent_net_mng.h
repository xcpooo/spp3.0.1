/**
 * @brief Nest agent net manager 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"


#ifndef __NEST_AGENT_NET_MNG_H_
#define __NEST_AGENT_NET_MNG_H_

using std::map;
using std::vector;

namespace nest
{
  
    /** 
     * @brief 网络远程交互连接器的全局管理
     */
    class  TAgentNetMng : public TNestDispatch
    {
    public:

        ///< 单例类访问函数
        static TAgentNetMng* Instance();

        ///< 全局的销毁接口
        static void Destroy();
        
        ///< 清理unix domain socket path
        static void RmUnixSockPath(uint32_t pid);

        ///< 检查 unix domain socket path
        static bool CheckUnixSockPath(uint32_t pid, bool abstract = false);

        ///< 判定目录是否存在
        static bool IsDirExist(const char* path);

        ///< 判定文件是否存在
        static bool IsFileExist(const char* path);

        ///< 析构清理函数
        virtual ~TAgentNetMng();

        ///< 初始设置服务基本信息
        int32_t InitService(uint32_t setid, TNestAddrArray& servers);

        ///< 更新服务的set信息, 触发重连
        int32_t UpdateSetInfo(uint32_t setid, TNestAddrArray& servers);

        ///< 获取本地监听的ip信息
        char* GetListenIp() {
            static char dflt[] = "127.0.0.1";
            if (_tcp_listen) {
                return _tcp_listen->GetListenAddr().IPString(NULL, 0);
            } else {
                return dflt;
            }
        };

        ///< 创建保持连接管理信息
        TNestChannel* InitKeepChannel();  

        ///< 获取保持连接管理信息
        TNestChannel* GetKeepChannel() {
            return _keep_conn;
        };

        ///< 查询动态连接信息
        TNestChannel* FindChannel(uint32_t fd);  

        ///< 连接检测处理函数
        int32_t CheckKeepAlive(); 

        ///< 重新选择server接口
        void ChangeServerAddr();

        ///< 发送报文到本地agent
        int32_t SendtoAgent(const void* data, uint32_t len);

        ///< 发送报文到本地
        int32_t SendtoLocal(TNestAddress* addr, const void* data, uint32_t len);

        ///< 发送并接收报文
        int32_t SendRecvLocal(TNestAddress* addr, const void* data, uint32_t len, 
                                  void* buff, uint32_t& buff_len, uint32_t time_wait);

        /**
         * @brief 分发处理函数
         */
        virtual int32_t DoRecv(TNestBlob* channel);
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief 发送数据包到代理server
         */
        int32_t SendToServer(void* buff, uint32_t len);
        
        /**
         * @brief 执行消息收发, 启动服务运行
         */
        void RunService();

    private:

        // 单例构造函数
        TAgentNetMng();
        
        static TAgentNetMng*        _instance;       // 单例对象
        
        TNestChannel*               _keep_conn;      // 保持长连接
        TNestListen*                _tcp_listen;     // 远程TCP socket监听
        TNestUdpListen*             _unix_listen;    // 本地程unix 报文 socket监听 
        TNestChannelMap             _channel_map;    // 连接管理map

        uint32_t                    _setid;          // setid
        TNestAddrArray              _server_addrs;   // server地址
        uint32_t                    _retry_count;    // 重试次数
        uint32_t                    _server_index;   // server 的位置
        TNestAddress                _server;         // 当前server
    };


}

#endif

