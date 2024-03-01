/*
 * =====================================================================================
 *
 *       Filename:  NetHandler.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/14/2010 11:34:20 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __NET_HANDLER_H__
#define __NET_HANDLER_H__

#include "CommDef.h"
#include <poller.h>
#include <timerlist.h>
#include <string>
#include <list>
#include <vector>
#include <sys/time.h>

BEGIN_ASYNCFRAME_NS

class CActionInfo;

class CNetHandler
    : public CPollerObject
    , public CTimerObject
{
    public:
        enum ConnState {
            UNCONNECTED = 0,
            CONNECTING,
            CONNECTED,
        };

        enum ProcessState {
            IDLE = 0,
            SENDING,
            RECVING,
            IDLE_RECYCLE, //空闲待回收状态
        };
        struct SActionNode
        {
            
            CActionInfo*   actionInfo; 
            // 进入pending 时候的actioninfo的时间, 单位(ms)
            int64_t        en_time;
            SActionNode(CActionInfo* ac, int64_t time)
            {
                actionInfo = ac;
                en_time = time;
            } 

        };

    public:
        CNetHandler(CPollerUnit *pollerUnit,
                        CTimerUnit *timerUnit,
                        const std::string &ip,
                        PortType port,
                        ProtoType proto,
                        ConnType conn_type,
						int TOS);

        virtual ~CNetHandler();

        // 处理请求
        virtual int HandleRequest(CActionInfo *req);

        // 获取目标IP
        inline void GetDestIP(std::string &ip) {
            ip = _dest_ip;
        }

        // 获取目标port
        inline void GetDestPort(PortType &port) {
            port = _dest_port;
        }

        // 获取协议类型
        inline void GetProtoType(ProtoType &proto) {
            proto = _proto;
        }

        // 获取连接类型
        inline void GetConnType(ConnType &type) {
            type = _conn_type;
        }

    protected:
        int SetNonBlocking(int fd);
        int Open();
        int Close();
        int DoConnect();
        int DoSend();
        int DoRecv();
        void DoSendFinished();
        void DoRecvFinished(const char* buf, int len);
        void DoErrorFinished(int err);
        void TryNextAction();
        int StartActionProcess();

        int GetTimeCost();

        void UpdateRouteResultIfNeed(int ecode);

    public: // CPollerObject
        virtual int InputNotify();
        virtual int OutputNotify();
        virtual int HangupNotify();

    public: // CTimerObject
        virtual void TimerNotify(void);
        //正常定时器定时器

        void EnableIdleTimer(void);

        void DisableIdleTimer(void);

        void EnableDoingTimer(int timeout);
        void DisableDoingTimer(void);
    public:
        void SetCanRecycle(); 
        bool CanRecycle();
        void Recycle();



    protected:
        std::string                 _dest_ip;
        PortType                    _dest_port;
        struct sockaddr_in          _dest_addr;
        ProtoType                   _proto;
        ConnType                    _conn_type;

        CActionInfo *               _action;
        std::list<SActionNode*>    _pending_action_list;
        CTimerUnit *                _timerUnit;
        CTimerList *                _timerList;
        CTimerList *                _timerListIdle;


        ConnState                   _conn_state;
        ProcessState                _proc_state;

        int                         _have_sent_len;
        int                         _have_recv_len;

        struct timeval              _start_tv;
		int                         _TOS;
};

END_ASYNCFRAME_NS

#endif
