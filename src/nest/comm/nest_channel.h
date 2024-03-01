/**
 * @brief nest channel 通道定义 
 */

#ifndef __NEST_CHANNEL_H_
#define __NEST_CHANNEL_H_

#include <stdint.h>
#include <time.h>
#include <list>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "global.h"
#include "tcache.h"
#include "tcommu.h"
#include "poller.h"
#include "nest_address.h"
#include "weight_schedule.h"


using std::vector;
using std::map;

using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;


namespace nest
{
    class TNestChannel;     
    class TNestAddress;    

    /**
     * @brief 消息结构分发定义
     */
    #define NEST_BLOB_TYPE_DGRAM            1   // 数据报类型    
    #define NEST_BLOB_TYPE_STREAM           2   // 数据流类型
    struct TNestBlob
    {
        uint32_t           _type;               // TCP/UDP 保留
        void*              _buff;               // 消息buff, 临时buff指针
        uint32_t           _len;                // 消息长度
        uint32_t           _recv_fd;            // 接收消息的fd
        uint64_t           _time_sec;           // 时间秒
        uint64_t           _time_usec;          // 微秒时间
        TNestAddress       _remote_addr;        // 对端地址
        TNestAddress       _local_addr;         // 本段地址
    };
    

    /**
     * @brief 通道交互分发处理, 长连接适用
     */
    class TNestDispatch
    {
    public:
        
        TNestDispatch() {};
        virtual ~TNestDispatch() {};

        // 实时触发收包, 一次处理完, 可不实现recvcache
        // 返回==0表明报文未收全, >0 表示成功处理长度, <0 失败
        virtual int32_t DoRecv(TNestBlob* blob)   { return 0;};
        
        // 主动检测收包, 可一次处理完, 也可每次定量处理
        // 返回==0表明报文未收全, >0 表示成功处理长度, <0 失败
        virtual int32_t DoRecvCache(TNestBlob* blob, TNestChannel* channel, bool block)   { return 0;};

        // 处理监听新连接接口, 返回0成功, 其他失败
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr) { return 0;};

        // 处理连接成功的通知
        virtual int32_t DoConnected(TNestChannel* channel) { return 0;};
 
        // 异常处理接口, 可直接回收channel指针
        virtual int32_t DoError(TNestChannel* channel) { return 0;};
    };    
    

    /**
     * @brief 长连接 channel连接状态定义
     */
    enum TNestConnState {
        UNCONNECTED = 0,
        CONNECTING  = 1,
        CONNECTED   = 2,
    };

    /**
     * @brief 长连接 channel class 定义
     */
    class TNestChannel : public CPollerObject, public IDispatchAble
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        TNestChannel();
        virtual ~TNestChannel();

        /**
         * @brief CPollerObject 对象事件处理函数
         */
        virtual int32_t InputNotify();
        virtual int32_t OutputNotify();
        virtual int32_t HangupNotify();

        /**
         * @brief 作为客户端, 主动发起连接, 封装连接细节
         * @return >0 连接成功, <0 连接失败, =0 连接进行中
         */
        int32_t Connect();

        /**
         * @brief 检查连接建立, 并执行连接过程, 建立事件管理
         * @return < 0 失败, > 0 已连接, =0 连接中
         */
        int32_t DoConnect();                

        /**
         * @brief 从TCP通道获取数据, 临时接收数据进入读缓冲区
         * @return >0 成功,返回接收的字节; <0 接收失败, =0 对端关闭
         */
        int32_t Recv();

        /**
         * @brief 满足每次只处理一个报文的需求, 提供缓存读取接口
         * @param  block 是否阻塞, true则一次取空, false 则每次取一个
         * @return 处理的报文数目, 小于0失败
         */
        int32_t RecvCache(bool block);

        /**
         * @brief 向TCP通道写入数据, 发送剩余的字段, 进入写缓冲区
         * @return >=0 成功,返回发送的字节; <0 失败
         */
        int32_t Send(const char* data, uint32_t data_len);
        int32_t SendEx(const char* ctrl, uint32_t ctrl_len, const char* data, uint32_t data_len);

        /**
         * @brief 继续发送写缓冲区的数据部分
         * @return >=0 成功,返回发送的字节; <0 失败
         */
        int32_t SendCache();

        /**
         * @brief 确认是否已连接
         */
        bool CheckConnected() {
            return (_conn_state == CONNECTED);
        };

        void SetConnState(TNestConnState state) {
            _conn_state = state;    
        };

        /**
         * @brief 设置远端地址
         */
        void SetRemoteAddr(struct sockaddr_in* addr) {
            _remote_addr.SetAddress(addr);
        };
        
        void SetRemoteAddr(TNestAddrType type, struct sockaddr* addr, socklen_t len) {
            _remote_addr.SetSockAddress(type, addr, len);
        }; 

        void SetRemoteAddr(TNestAddress* addr) {
            _remote_addr = *addr;
        };

        /**
         * @brief 获取远端IP字符串
         */
        char* GetRemoteIP() {
            return _remote_addr.IPString(NULL, 0);
        };

        TNestAddress& GetRemoteAddr() {
            return _remote_addr;
        };
        
        /**
         * @brief 设置通知分发对象
         */
        void SetDispatch(TNestDispatch* disp) {
            _dispatch = disp;
        };

        /**
         * @brief 内部对象管理
         */
        ConnCache* GetRwCache() {
            return _cc;
        }; 
        
        CPollerUnit* GetPollerUnit() {
            return ownerUnit;
        };

        /**
         * @brief 网络IO管理, socket句柄管理
         */
        void SetSockFd(int32_t fd) {
            netfd = fd;
        };        
        int32_t GetSockFd() {
            return netfd;
        };

        /**
         * @brief cookie管理
         */
        void SetCookie(void* arg) {
            _cookie = arg;
        };        
        void* GetCookie() {
            return _cookie;
        };
        

        /**
         * @brief 创建socket句柄, 默认为流式
         */
        int32_t CreateSock(int32_t domain, int32_t type = SOCK_STREAM);

    protected:
    
        ConnCache*              _cc;           // send recv cache mng
        TNestConnState          _conn_state;   // connect state
        TNestAddress            _remote_addr;  // 远端地址
        TNestDispatch*          _dispatch;     // 分发通知
        void*                   _cookie;       // 私有指针信息
        
    };
    
    typedef std::map<uint32_t, TNestChannel*>  TNestChannelMap;
    typedef std::vector<TNestChannel*>         TNestChannelList;

    /**
     * @brief 长连接 listen class 定义
     */
    class TNestListen : public CPollerObject
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        TNestListen() {
            _dispatch = NULL;    
			_tcp = true;
        };
        virtual ~TNestListen() {};

        /**
         * @brief CPollerObject 对象事件处理函数
         */
        virtual int32_t InputNotify();
        virtual int32_t OutputNotify();
        virtual int32_t HangupNotify();
        
        /**
         * @brief 初始化与终止函数
         */
        virtual int32_t Init();
        virtual void Term() { return;};

        /**
         * @brief 初始化listen socket
         */
        virtual int32_t CreateSock();  

        /**
         * @brief 设置地址监听信息
         */
        void SetListenAddr(struct sockaddr_in* addr) {
            _listen_addr.SetAddress(addr);
        };
        void SetListenAddr(struct sockaddr_un* addr, bool abstract = false) {
            _listen_addr.SetAddress(addr, abstract);
        };
        void SetListenAddr(TNestAddress* addr) {
            _listen_addr = *addr;
        };   
        TNestAddress& GetListenAddr() {
            return _listen_addr;
        };

        /**
         * @brief 设置通知分发对象
         */
        void SetDispatch(TNestDispatch* disp) {
            _dispatch = disp;
        };  

        /**
         * @brief 网络IO管理, socket句柄管理
         */
        int32_t GetSockFd() {
            return netfd;
        };

		bool IsTcp()
		{ 
			return _tcp;
		}
    protected:
    
        TNestAddress     _listen_addr;        // listen address
        TNestDispatch*   _dispatch;           // 分发通知 
        bool             _tcp;                   
    };


    /**
     * @brief udp listen class 定义
     */
    class TNestUdpListen : public TNestListen
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        TNestUdpListen();
        virtual ~TNestUdpListen();
    
        /**
         * @brief CPollerObject 对象事件处理函数
         */
        virtual int32_t InputNotify();

        /**
         * @brief 初始化listen socket
         */
        virtual int32_t CreateSock();  

    protected:

        char*               _msg_buff;              // 临时接收缓冲
        uint32_t            _buff_size;             // 缓冲大小

    };
   

    /** 
     * @brief 网络远程交互连接器的全局管理
     */
    class  TNestNetComm
    {
    public:

        /** 
         * @brief 析构函数
         */
        ~TNestNetComm();

        /** 
         * @brief 单例类句柄访问接口
         */
        static TNestNetComm* Instance();

        /** 
         * @brief 全局的销毁接口
         */
        static void Destroy();

        /** 
         * @brief 内存池与epoll管理访问接口
         */
        CMemPool* GetMemPool() {
            return _mem_pool;
        };
        
        CPollerUnit* GetPollUnit() {
            return _poll_unit;
        };
        
        void SetPollUnit(CPollerUnit* poll) {
            _poll_unit = poll;
        };

        /**
         * @brief 创建与销毁一个poll unit对象
         */
        CPollerUnit* CreatePollUnit() ;   
        
        void DestroyPollUnit(CPollerUnit* unit);

        /** 
         * @brief 全局的epoll unit执行接口
         */
        void RunPollUnit(uint32_t wait_time);


    private:

        /** 
         * @brief 构造函数
         */
        TNestNetComm();
    
        static TNestNetComm *_instance;         // 单例类句柄 
        
        CPollerUnit*         _poll_unit;        // epoll管理对象
        CMemPool*            _mem_pool;         // 内存池对象
    };

}

#endif

