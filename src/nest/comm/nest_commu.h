/**
 * @brief net commu for proxy worker
 */

#ifndef _TNEST_COMMU_H_
#define _TNEST_COMMU_H_

#include <stdint.h>
#include <time.h>
#include "nest_stat.h"
#include "nest_channel.h"
#include "nest_context.h"

#define NEST_MAGIC_NUM          0x7473656e      // 校验幻数 magic num 'nest'
#define NEST_PROXY_BASE_PORT    61001           // 基准侦听端口
#define NEST_PROXY_PRIMER       3989            // 基准的映射质数
#define NEST_HEARBEAT_TIMES     6000            // 6K次心跳 约1分钟 心跳一次

namespace nest
{

    /**
     * @brief 内部消息类型细分定义
     */
    enum 
    {
        NEST_MSG_TYPE_USER_DATA       = 0,      // 用户数据消息
        NEST_MSG_TYPE_REGIST          = 1,      // 注册消息, 启动时刻一次
        NEST_MSG_TYPE_HEARTBEAT       = 2,      // 心跳消息, 主动识别断开
        NEST_MSG_TYPE_UNREGIST        = 3,      // 取消注册, 优雅退出
    };


    /**
     * @brief 内部交互的配置结构
     */
    struct TNetCoummuConf
    {
        int32_t             route_id;   // 路由id    
        TNestAddress        address;    // 监听地址
    };
    
    /**
     * @brief 通道交互分发处理, 长连接适用
     */
    class TCommuDisp : public TNestDispatch, public CTCommu
    {
    public:

        ///< 虚析构函数
        virtual ~TCommuDisp() {};

        ///< 继承Commu对象的接口函数
        virtual int init(const void* config) { return 0;};
        virtual int poll(bool block = false) { return 0;};
        virtual int sendto(unsigned flow, void* arg1, void* arg2){ return 0;};
        virtual int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2){ return 0;};
        virtual int clear() { return 0;};
        virtual void fini() { return;};

        ///< 公共消息头的字节序转换函数
        static void ntoh_msg_head(TNetCommuHead* head_info);
        static void hton_msg_head(TNetCommuHead* head_info);

        ///< CRC生成及检查函数
        static bool check_crc(TNetCommuHead* head_info);
        static uint32_t generate_crc(TNetCommuHead* head_info);

        ///< 默认设置, 起始时间为0, 不进行时延与应答统计
        void default_msg_head(TNetCommuHead* head_info, uint32_t flow, uint32_t group_id);

        /** 
         * @brief 检查消息的格式, 返回报文长度, 头长度
         * @param blob 消息块
         * @param head_len  头长度, 返回>0时有效
         * @return 消息长度, 0 -待继续接收; >0 消息长度; <0无效报文
         */
        int32_t CheckMsg(void* pos, uint32_t left, TNetCommuHead& head_info);

        /**
         * @brief  适配原有的处理流程, 先接收, 后处理
         */
        virtual int32_t DoRecvCache(TNestBlob* blob, TNestChannel* channel, bool block);

        /**
         * @brief 业务消息接收后处理函数, 虚接口函数
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info){};

        /**
         * @brief 业务消息处理函数, 虚接口函数
         */
        virtual void HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);

        /**
         * @brief 控制消息处理函数, 虚接口函数
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body) = 0;
        
    };   
    

    /** 
     * @brief 服务器端侦听的分发管理类
     */
    class TCommuSvrDisp : public TCommuDisp
    {
    public:

        ///< 虚析构函数
        virtual ~TCommuSvrDisp();

        /**
         * @brief 继承自disp的接口函数实现
         */
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief 业务消息接收后处理函数, 虚接口函数
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info);

        /**
         * @brief 控制消息处理函数, 虚接口函数
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);
        
        /**
         * @brief 管理新连接的通道信息
         */
        void AddChannel(TNestChannel* channel);

        /**
         * @brief 清理连接的通道信息
         */
        void DelChannel(TNestChannel* channel);
        
        /**
         * @brief 管理新连接的通道信息
         */
        TNestChannel* FindChannel(uint32_t fd);

        /**
         * @brief 注册连接的通道信息
         */
        int32_t RegistChannel(TNestChannel* channel, uint32_t group);

        /**
         * @brief 检查已有连接, 未分组的注册情况
         */
        uint32_t CheckUnregisted();

        /**
         * @brief 设置通用地址监听, 成功返回0, 失败小于0
         */
        int32_t StartListen(TNestAddress& address);

        /**
         * @brief 启动本地侦听, 返回侦听端口
         */
        uint32_t StartListen(uint32_t sid, uint32_t pno, uint32_t addr);

    protected:
    
        TNestListen            _listen_obj;     // listen obj
        TNestChannelMap        _channel_map;    // 全局的连接管理map
        TNestChannelMap        _un_rigsted;     // 临时map记录未注册的连接
    };


    /** 
     * @brief 网络远程交互连接器的服务器端
     */
    class TNetCommuServer : public CTCommu, public CWeightSchedule, public CDynamicCtrl
    {
    public:
    
        /**
         * @brief 构造与析构函数
         */
        TNetCommuServer(uint32_t group) : _groupid(group) {};
        virtual ~TNetCommuServer(){};
    
        /**
         * @brief 继承自CTCommu的虚函数
         */
        int32_t init(const void* config);
        int32_t clear(){return 0;};
        int32_t poll(bool block = false);
        int32_t sendto(uint32_t flow, void* arg1, void* arg2);
        int32_t ctrl(uint32_t flow, ctrl_type type, void* arg1, void* arg2){return 0;};
        void fini() {};
    
        /**
         * @brief 连接对象channel管理函数
         */
        void AddChannel(TNestChannel* channel);
        void DelChannel(TNestChannel* channel);

        /**
         * @brief 轮循算法管理
         */
        TNestChannel* FindNextChannel();

        /**
         * @brief 获取groupid信息
         */
        uint32_t GetGroupId() {
            return _groupid;
        };
        

    protected:

        uint32_t                 _groupid;           // 分组id
        TNestChannelList         _channel_list;      // channel list
    };
    
    typedef std::map<int32_t, TNetCommuServer*> TNetCommuSvrMap;


    /** 
     * @brief 客户端侦听的分发管理类
     */
    class TCommuCltDisp : public TCommuDisp
    {
    
    public:
    
        ///< 虚析构函数
        virtual ~TCommuCltDisp() {};

        /**
         * @brief 继承自disp的接口函数实现
         */
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief 处理已连接的通知接口
         */
        virtual int32_t DoConnected(TNestChannel* channel);

        /**
         * @brief 业务消息接收后处理函数, 虚接口函数
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info);
        
        /**
         * @brief 业务消息处理函数, 虚接口函数
         */
        virtual void HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);
        
        /**
         * @brief 控制消息处理函数, 虚接口函数
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body) {};

        /**
         * @brief 存储消息信息上下文
         */
        static bool SaveMsgCtx(TNetCommuHead& head_info);

        /**
         * @brief 获取消息信息上下文
         */
        static bool RestoreMsgCtx(uint32_t flow, TNetCommuHead& head_info, uint64_t& save_time);

    };


    /** 
     * @brief 网络远程交互连接器的客户端
     */
    class TNetCommuClient : public CTCommu
    {
    public:
    
        /**
         * @brief 构造与析构函数
         */
        TNetCommuClient();
        virtual ~TNetCommuClient();
    
        /**
         * @brief 继承自CTCommu的虚函数
         */
        int32_t init(const void* config);
        int32_t clear() {return 0;};
        int32_t poll(bool block = false);
        int32_t sendto(uint32_t flow, void* arg1, void* arg2);
        int32_t ctrl(uint32_t flow, ctrl_type type, void* arg1, void* arg2){return 0;};
        void fini() {};

        /**
         * @brief 连接保活管理
         */
        int32_t CheckConnect();
        int32_t CreateSock();

        /**
         * @brief 初始化连接
         */
        TNestChannel* InitChannel();

        /**
         * @brief 重启连接
         */
        bool RestartChannel();

        /**
         * @brief 通道管理的客户端主动心跳逻辑
         */
        int32_t Heartbeat();
        void CheckHeartbeat();
        
        /**
         * @brief 分组ID管理
         */
        uint32_t GetGroupId() {
            return _net_conf.route_id;
        };

    protected:

        TNestChannel*             _conn_obj;      // n:1 连接对象
        TNetCoummuConf            _net_conf;      // 配置信息

    };


    /** 
     * @brief 网络远程交互连接器的全局管理
     */
    class  TNetCommuMng
    {
    public:

        /** 
         * @brief 析构函数
         */
        ~TNetCommuMng();

        /** 
         * @brief 单例类句柄访问接口
         */
        static TNetCommuMng* Instance();

        /** 
         * @brief 全局的销毁接口
         */
        static void Destroy();

        /** 
         * @brief 内部通讯接口查询与访问接口
         */
        TNetCommuServer* GetCommuServer(int32_t route_id);
        
        TNetCommuClient* GetCommuClient();

        /** 
         * @brief 内部通讯接口管理注册接口
         */
        void RegistClient(TNetCommuClient* client);
        
        void RegistServer(int32_t route, TNetCommuServer* server);

        /**
         * @brief 获取分发通知接口
         */
        TCommuSvrDisp& GetCommuSvrDisp() {
            return _svr_disp;
        };

        TCommuCltDisp& GetCommuCltDisp() {
            return _clt_disp;
        };

        /** 
         * @brief 内部交互通道保活检测接口
         */
        void CheckChannel();

        /** 
         * @brief 内部消息上下文队列检测
         */
        void CheckMsgCtx(uint64_t now, uint32_t wait);

        /**
         * @brief 设置通知对象, 代理通知执行
         */
        void SetNotifyObj(void* obj) {
            _notify_obj = obj;
        };

        /**
         * @brief 添加epoll关联通知对象
         */
        void AddNotifyFd(int32_t fd);
        void DelNotifyFd(int32_t fd);

        /**
         * @brief 获取统计的接口对象
         */
        CMsgStat* GetMsgStat() {
            return &_stat_obj;
        }; 

        /**
         * @brief 获取消息上下文管理的接口对象
         */
        CMsgCtxMng& GetMsgCtxMng() {
            return _msg_ctx;
        }; 

        /**
         * @brief 获取client的连接句柄
         */
        int32_t GetClientFd() {
            return _client_fd;
        };
        
        /**
         * @brief 设置client的连接句柄
         */
        void SetClientFd(int32_t fd) {
            _client_fd = fd;
        };

    private:

        /** 
         * @brief 构造函数
         */
        TNetCommuMng() ;
    
        static TNetCommuMng *_instance;         // 单例类句柄 
        
        TNetCommuSvrMap        _server_map;     // 多个连接对象按路由注册
        TNetCommuClient*       _client;         // 每worker一个连接对象
        
        TCommuSvrDisp          _svr_disp;       // server的分发通知处理 
        TCommuCltDisp          _clt_disp;       // client的分发通知处理
        int32_t                _client_fd;      // 作为客户端, 连接fd可以作为通知fd

        void*                  _notify_obj;     // 注册的通知句柄, server需要
        CMsgStat               _stat_obj;       // 统计接口 
        CMsgCtxMng             _msg_ctx;        // 消息上下文管理
        
    };


}

#endif

