/**
 * @file NetMgr.h
 * @brief 网络管理模块
 * @author aoxu, aoxu@tencent.com
 * @version 1.1
 * @date 2011-10-09
 */
#ifndef __NETMGR_H__
#define __NETMGR_H__

#include "CommDef.h"
#include "PtrMgr.h"
#include "ParallelNetHandler.h"

#include <map>
#include <set>

class CPollerUnit;
class CTimerUnit;

BEGIN_ASYNCFRAME_NS

class CNetHandler;
class CAsyncFrame;

// CNetHandler索引
typedef struct handler_idx
{
    char        _ip[18]; // 所引用的宏已暴露给用户，所以把宏改为常数
    PortType    _port;
    ProtoType   _proto;

    bool operator<(const handler_idx& right)const;
}handler_idx_t;

// CNetHandler数组
typedef struct handler_array
{
    unsigned _cur_num;
    unsigned _max_num;
    CNetHandler **_aryHandler;

    handler_array(const unsigned max_num);
    ~handler_array();
}handler_array_t;

// CParallelNetHandler数组
typedef struct parallel_handler_array
{
    unsigned _cur_num;
    unsigned _max_num;
    CParallelNetHandler **_aryHandler;

    parallel_handler_array(const unsigned max_num);
    ~parallel_handler_array();
}parallel_handler_array_t;

class CNetMgr
    : public CTimerObject
{
    friend class CNetHandler;
    friend class CParallelNetHandler;

    public:
        static CNetMgr* Instance (void);
        static void Destroy (void);

        CNetMgr();
        ~CNetMgr();

        /**
         * 初始化
         *
         * @param pollerUnit CPollerUnit对象指针
         * @param timerUnit CTimerUnit对象指针
         * @param pFrame CAsyncFrame对象指针
         * @param max_long_conn_num 允许pending的连接个数
		 * @param TOS socket TOS值
         *
         * @return 0: 成功；其他：失败
         *
         */
        int Init(CPollerUnit *pollerUnit, 
                    CTimerUnit *timerUnit, 
                    CAsyncFrame *pFrame,
                    int PendingConnNum,
					int TOS);

        /**
         * 反初始化
         */
        void Fini();

        /**
         * 获取CNetHandler指针对象
         *
         * @param ip ip地址
         * @param port 端口
         * @param proto 协议类型
         * @param conn_type 连接类型
         *
         * @return 返回CNetHandler指针对象
         */
        CNetHandler *GetHandler(const std::string &ip,
                                PortType port, 
                                ProtoType proto,
                                ConnType conn_type); 
        /**
         * 获取CParallelNetHandler指针对象
         *
         * @param ip ip地址
         * @param port 端口
         * @param proto 协议类型
         * @param conn_type 连接类型
         *
         * @return 返回CParallelNetHandler指针对象
         */
        CParallelNetHandler *GetParallelHandler(const std::string &ip,
                                PortType port, 
                                ProtoType proto,
                                ConnType conn_type); 

        /**
         * 回收CNetHandler指针对象
         *
         * @param pHandler 待回收的CNetHandler指针对象
         */
        void RecycleHandler(CNetHandler *pHandler);
        
        /**
         * @brief 对一组ip/port设置连接数上限
         *
         * @param ip 后端服务器IP地址
         * @param port 后端服务器端口号
         * @param limit 连接上限数（如果为0则使用默认值10）
         */
        void InitConnNumLimit(const std::string&ip, const unsigned short port, const unsigned int limit);

        /**
         * @brief 取一组ip/port的连接数上限
         *
         * @param ip 后端服务器IP地址
         * @param port 后端服务器端口号
         *
         * @return 当前连接数上限
         */
        unsigned GetConnNumLimit(const std::string&ip, const unsigned short port);
        
        /**
         * @brief 供内部调用，查找handler_idx_t设置连接数上限
         *
         * @param idx 指定一组ip/port的索引结构
         * @param limit 连接数上限
         */
        void InitConnNumLimit(const handler_idx_t& idx, const unsigned int limit);

        /**
         * @brief 供内部调用，查找handler_idx_t取连接数上限
         *
         * @param idx 指定一组ip/port的索引结构
         *
         * @return 当前连接数上限
         */
        unsigned GetConnNumLimit(const handler_idx_t& idx);
    protected:
        void TimerNotify(void);

        void CheckIdleHander(void);

    private:
        typedef std::map<handler_idx_t, std::set<CNetHandler*> > HandlerMap;
        typedef std::map<handler_idx_t, handler_array_t* > PendingHandlerMap;
        typedef std::map<handler_idx_t, parallel_handler_array_t* > ParallelHandlerMap;

        static CNetMgr * _instance;

        CPollerUnit *_pollerUnit;
        CTimerUnit *_timerUnit;
        CTimerList* _timerList;
        CAsyncFrame *_pFrame;
        CPtrMgr<CNetHandler, PtrType_New> *_nhptr_mgr;
        int _MaxPendingConnNum;
        int _TOS;

        // 不能pending请求的CNetHandler
        HandlerMap _mapHandler;

        // 允许pending请求的CNetHandler
        PendingHandlerMap _mapPendingHandler;

        // 多发多收请求的CParallelNetHandler
        ParallelHandlerMap _mapParallelHandler;
};

END_ASYNCFRAME_NS

#endif
