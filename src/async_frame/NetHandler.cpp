/*
 * =====================================================================================
 *
 *       Filename:  NetHandler.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/14/2010 11:34:17 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "NetHandler.h"
#include "ActionInfo.h"
#include "AsyncFrame.h"
#include "NetMgr.h"
#include "RouteMgr.h"
#include "../comm/exception.h"
#include "../comm/monitor.h"
#include <misc.h>
#include <fcntl.h>


USING_ASYNCFRAME_NS;
USING_L5MODULE_NS;
using namespace spp::exception;
using namespace spp::comm;

#define IDLE_RECYCLE_TIMEOUT 3600 //modify to 60min in 2.10.6

int g_spp_L5us;   // modify add switch  2.10.8	release 2

#define FramePtr CNetMgr::Instance()->_pFrame
#define MsgPtr (_action ? _action->_pMsg : NULL)

CNetHandler::CNetHandler(CPollerUnit *pollerUnit,
        CTimerUnit *timerUnit,
        const std::string &ip,
        PortType port,
        ProtoType proto,
        ConnType conn_type,
		int TOS)
    : CPollerObject(pollerUnit), _dest_ip(ip),_dest_port(port)
    , _proto(proto), _conn_type(conn_type)
    , _action(NULL), _timerUnit(timerUnit), _timerList(NULL)
    , _conn_state(UNCONNECTED), _proc_state(IDLE), _have_sent_len(0), _have_recv_len(0)
{
    _dest_addr.sin_family = AF_INET;
    inet_aton(_dest_ip.c_str(), &(_dest_addr.sin_addr));
    _dest_addr.sin_port = htons(_dest_port);
    _TOS = TOS;
    _timerListIdle = timerUnit->GetTimerList(IDLE_RECYCLE_TIMEOUT * 1000);
    FramePtr->FRAME_LOG(LOG_TRACE, "new CNetHandler: %p -(%s, %u, %s)\n",
            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto) );
}

CNetHandler::~CNetHandler()
{
    FramePtr->FRAME_LOG(LOG_TRACE, "delete CNetHandler: %p -(%s, %u, %s)\n",
            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto) );

    if (_action != NULL)
    {
        //never go here
        FramePtr->FRAME_LOG(LOG_ERROR, "action is no null, will cause mem leak!!!\n"
                        );
    }

    while (!_pending_action_list.empty())
    {
        //never go here
        FramePtr->FRAME_LOG(LOG_ERROR, "delete action, will cause mem leak!!!\n"
                );
        delete _pending_action_list.back();
        _pending_action_list.pop_back();
    }
    Close();
}

int CNetHandler::HandleRequest(CActionInfo *action)
{

    if( _action != NULL &&
            ( _conn_type == ConnType_Short
              || _conn_type == ConnType_Long) )
    {// never go here
        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): Invalid State: "
                                            "No Pending Conn But Processing another action \n",
                                            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto) );
        return -1; // 正在处理
    }
    else if( _action == NULL )
    {// 没有正在处理的action，马上执行当前action
        _action = action;

        return StartActionProcess();
    }
    else
    {// 正在处理action，将当前action放入等待队列中
        SActionNode* actionNode = new SActionNode(action, __spp_get_now_ms() );
        if (actionNode == NULL)
        {
            FramePtr->FRAME_LOG( LOG_ERROR,"%p - (%s, %u, %s): CActionInfo:%p, new SActionNode failed!!!!\n",
                    this, action, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto));
        }
        _pending_action_list.push_back(actionNode);
    }

    return 0;
}

int CNetHandler::Open()
{
    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): create socket. Msg: %p\n",
                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

    // 创建socket fd
    if( netfd <= 0 ) {
        if( _proto == ProtoType_TCP ) {
            netfd = socket(AF_INET, SOCK_STREAM, 0);
        } else {
            netfd = socket(AF_INET, SOCK_DGRAM, 0);
        }

        if(netfd <= 0) {
            MONITOR(MONITOR_WORKER_SOCKET_FAIL); //create socket failed
            DoErrorFinished( ECreateSock );
            return -1;
        }
    }

    if(_TOS >= 0)
	    setsockopt(netfd, IPPROTO_IP, IP_TOS, &_TOS, sizeof(int));

    // 将socket fd设置为非阻塞
    SetNonBlocking(netfd);

    // 打开Output/Input事件
    EnableOutput();
    EnableInput();

    // 绑定到PollerUnit
    int ret = -1;
    ret = AttachPoller();
    if(-1 == ret) {
        DoErrorFinished( EAttachPoller );
        return -1;
    }

    if ( _proto == ProtoType_TCP )
    { // tcp
        // 开始连接后端server, 通常在OutputNotify事件第一次触发时，表明连接成功
        if( 0 != DoConnect() ) {
            DoErrorFinished( EConnect );
            return -1;
        }

    }
    else
    { // udp
        _conn_state = CONNECTED;
    }

    return 0;
}
int CNetHandler::Close()
{
    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): Close.\n",
                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto) );

    DisableInput();
    DisableOutput();
    DisableTimer();

    _conn_state = UNCONNECTED;
    _proc_state = IDLE;

    int ret = DetachPoller();
    if(-1 == ret)
    {
    }

    // 关闭socket fd
    if(netfd > 0)
    {
        ::close(netfd);
        netfd = -1;
    }

    return 0;
}
int CNetHandler::SetNonBlocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)   {
        return -1;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)   {
        return -1;
    }

    return 0;
}


int CNetHandler::DoConnect()
{
    int ret = connect(netfd, (sockaddr *)&_dest_addr, sizeof(_dest_addr));
    if(ret == -1)
    {
        switch(errno)
        {
            case EAGAIN:
            case EINTR :
            case EINPROGRESS :
                _conn_state = CONNECTING;
                break;
            default:
                FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): connect failed, Msg: %p, errno: %m\n",
                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
                MONITOR(MONITOR_WOKER_CONNECT_FAIL);//socket connect failed
                return -1;
        }
    }
    else
    {
        FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): connect succ, Msg: %p\n",
                this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        _conn_state = CONNECTED;
    }
    return 0;
}


int CNetHandler::InputNotify()
{
    if( netfd <= 0 )
    {
        DoErrorFinished( EInvalidState );
        return -1;
    }

    if( IDLE == _proc_state )
    { //在空闲状态下对方关闭连接, 有可能进入此分支

        FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): InputNotify when idle.\n",
                                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto) );
        Close();
        //由于close会detach timer
        //重新挂回idle timer
        EnableIdleTimer();
        TryNextAction();
        return 0;
    }

    if( _proc_state != RECVING )
    {
        DoErrorFinished( EInvalidState );
        return 0;
    }


    if( NULL == _action )
    {// never go here
        Close();
        EnableIdleTimer();
        TryNextAction();
        return 0;
    }

    // 调用DoRecv来接收来自后端server的数据，并处理
    return DoRecv();
}

int CNetHandler::OutputNotify()
{
    if( netfd <= 0 )
    {
        DoErrorFinished( EInvalidState );
        return -1;
    }

    if(_conn_state == CONNECTING)
    {
    	int error = 0;
    	socklen_t len = sizeof(error);
		if (::getsockopt(netfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error!=0) 
		{
	        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): connect failed (%m), Msg: %p\n",
                                this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);		
	        DoErrorFinished( EConnect);
	        return -1;			
		}

        // 连接成功
        FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): connect succ, Msg: %p\n",
                                this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        _conn_state = CONNECTED;
    }

    if(_conn_state != CONNECTED)
    {
        DoErrorFinished( EInvalidState );
        return -1;
    }

    if( _proc_state != SENDING )
    {
        DoErrorFinished( EInvalidState );
        return 0;
    }

    if( NULL == _action )
    { // no action request to send
        DoErrorFinished( EInvalidState );
        return 0;
    }

    // 调用DoSend发送请求数据
    return DoSend();

}

int CNetHandler::HangupNotify()
{
    FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): Hangup, Msg: %p\n",
                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
    DoErrorFinished( EHangup );
    return 0;
}

void CNetHandler::TimerNotify(void)
{
    if (_proc_state == IDLE_RECYCLE)
    {
        //never go here
        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): invalid state Timeout, Msg: %p\n",
                this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
        return;
    }
    if (_proc_state != IDLE)
    {
        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): Timeout, Msg: %p\n",
                this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        DoErrorFinished( ETimeout );
    }
    else
    {
        //if (_conn_type == ConnType_Short || _conn_type == ConnType_Long)
        {
            FramePtr->FRAME_LOG( LOG_DEBUG, "%p - (%s, %u, %s): idle Timeout, Msg: %p\n",
                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
            SetCanRecycle();

        }


    }
}

int CNetHandler::DoSend()
{
    char *data = _action->_buf;
    int total_len = _action->_real_data_len;

    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): send, Msg: %p, total_send_size: %d, have_sent_len: %d\n",
                                            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto),
                                            MsgPtr, total_len, _have_sent_len);

    int ret = -1;
    if ( _proto == ProtoType_TCP )
    {
        ret = ::send (netfd,
                        data + _have_sent_len ,
                        total_len - _have_sent_len,
                        0);
    }
    else
    {
        ret = ::sendto(netfd,
                        data + _have_sent_len ,
                        total_len - _have_sent_len,
                        0,
                        (struct sockaddr *)&_dest_addr, sizeof(struct sockaddr_in));

    }

    if(-1 == ret)
    {
        if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
        {
            EnableOutput();
            ApplyEvents();
            return 0;
        }

        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): send failed, Msg: %p, errno: %m\n",
                                             this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        MONITOR(MONITOR_WORKER_SEND_FAIL); //send failed

        DoErrorFinished( ESend );

        return -1;
    }

    _have_sent_len += ret;

    if( total_len == _have_sent_len)
    {// send finished
        DisableOutput();
        ApplyEvents();
        DoSendFinished();
    }
    else
    {
        EnableOutput();
        ApplyEvents();
    }

    return 0;
}
int CNetHandler::DoRecv()
{
    char *buf = _action->_buf;
    int buf_size = _action->_buf_size;

    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): recv data, Msg: %p, have_recv_len: %d\n",
                                            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto),
                                            MsgPtr, _have_recv_len);
    int ret = -1;
	socklen_t addr_len = sizeof(sockaddr_in);
    struct sockaddr_in stInAddr;
    if ( _proto == ProtoType_TCP )
    {
        ret = ::recv( netfd, buf+_have_recv_len, buf_size-_have_recv_len, 0 );
    }
    else
    {
        ret = ::recvfrom( netfd, buf+_have_recv_len, buf_size-_have_recv_len, 0, (struct sockaddr *)&stInAddr, &addr_len);
    }

    if(-1 == ret)
    {
        if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
        {
            return 0;
        }

        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): recv failed, Msg: %p, errno: %m\n",
                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        DoErrorFinished( ERecv );

        return -1;
    }
    else if( ret == 0 )
    {
        FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): remote peer close socket, Msg: %p\n",
                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);

        DoErrorFinished( EShutdown );

        return -1;
    }

    _have_recv_len += ret;

    int pkg_len = _action->HandleInput( buf, _have_recv_len );
    if( pkg_len >= 0 && pkg_len <= _have_recv_len )
    {
        if( pkg_len > 0 )
        {// 接收到一个完整的响应包

		    if (_spp_g_exceptionpacketdump)
            {//保存数据包
                if ( _proto == ProtoType_TCP )
                {
                    PacketData stPackData(buf, pkg_len, _dest_addr.sin_addr.s_addr, 0, _dest_port, 0, __spp_get_now_ms());
                    SavePackage(PACK_TYPE_BACK_RECV, &stPackData);
                }
                else
                {
                    PacketData stPackData(buf, pkg_len, stInAddr.sin_addr.s_addr, 0, _dest_port, 0, __spp_get_now_ms());
                    SavePackage(PACK_TYPE_BACK_RECV, &stPackData);
                }
            }
					
            DoRecvFinished(buf, pkg_len);
        }
        else
        { // 继续接收数据
		
            //当udp协议时，如接收缓冲区已满，则执行异常流程
	    if( _have_recv_len == buf_size && ProtoType_UDP == _proto )
	    {
                FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): recv fail : no enough memory, Msg: %p, errno: %m\n",
                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
                DoErrorFinished(ERecvUncomplete);
	    }
        }
    }
    else
    { // 包完整性检查出错
        DoErrorFinished( ECheckPkg );
        return -1;
    }

    return 0;
}

void CNetHandler::DoErrorFinished(int err)
{// 出错处理

    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): error happens, Msg: %p, errno: %d\n",
                            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr, err);
    if( NULL == _action )
    {
        Close();
        EnableIdleTimer();
        TryNextAction();
        return;
    }

    // 备份_action, 用来调用HandleError, 调用这个方法之前必须把连接状态重置
    CActionInfo *action = _action;

    // 计算处理时间开销
    int cost = GetTimeCost();
    _action->SetTimeCost( cost );

    // 如果需要，上报路由访问结果
    UpdateRouteResultIfNeed( err );

    // 重置处理状态
    _proc_state = IDLE;
    _action = NULL;

    // 出错情况下，关闭当前连接
    Close();
    //挂到idle定时器
    //如果还有下一个请求, idle定时器会被取消
    EnableIdleTimer();

    // 尝试处理下一个请求
    TryNextAction();

    // 告诉上层请求处理结束
    action->HandleError(err);
}

int CNetHandler::GetTimeCost()
{
    __spp_get_now_ms();
    struct timeval tv = __spp_g_now_tv;

    int cost = (tv.tv_sec - _start_tv.tv_sec) * 1000;

    if( tv.tv_usec > _start_tv.tv_usec )
    {
        cost += (tv.tv_usec-_start_tv.tv_usec) / 1000;
    }
    else
    {
        cost -= (_start_tv.tv_usec-tv.tv_usec) / 1000;
    }

    if( cost < 0 ) {
        cost = 0;
    }


    return cost;
}

void CNetHandler::DoSendFinished()
{
    _have_sent_len = 0;
    _have_recv_len = 0;

    if( _action->SendOnly() )
    {// 无需接收响应包，当前Action处理完毕

        FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): send pkg succ, only send type, Msg: %p\n",
                                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr );

        // 备份_action, 用来调用HandleSendFinished, 调用这个方法之前必须把连接状态重置
        CActionInfo *action = _action;

        // 计算处理时间开销
        int cost = GetTimeCost();
        _action->SetTimeCost( cost );

        // 如果需要，上报路由访问结果
        UpdateRouteResultIfNeed( 0 );

        // 重置处理状态
        _proc_state = IDLE;
        _action = NULL;

        if(_conn_type == ConnType_Short  )
        {// 如果是短连接，则关闭当前连接
            Close();
        }
        //挂到idle定时器
        DisableDoingTimer();
        EnableIdleTimer();


        // 尝试处理下一个请求
        TryNextAction();

        // 告诉上层请求处理结束
        action->HandleSendFinished();
    }
    else
    {// 开始接收响应包
        FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): send pkg succ, begin to recv data, Msg: %p\n",
                                        this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr );

        _proc_state = RECVING;

        _action->ResetBuffer();
    }

}

void CNetHandler::DoRecvFinished(const char* buf, int len)
{
    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): recv pkg succ, pkg_len: %d, Msg: %p\n",
                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), len, MsgPtr );

    _have_sent_len = 0;
    _have_recv_len = 0;

    // 备份_action, 用来调用HandleProcess, 调用这个方法之前必须把连接状态重置
    CActionInfo *action = _action;

    // 计算处理时间开销
    int cost = GetTimeCost();
    _action->SetTimeCost( cost );

    // 如果需要，上报路由访问结果
    UpdateRouteResultIfNeed( 0 );

    // 重置处理状态
    _action = NULL;
    _proc_state = IDLE;

    DisableOutput();
    ApplyEvents();

    if(_conn_type == ConnType_Short )
    {// 如果是短连接，则关闭当前连接
        Close();
    }

    //挂到idle定时器
    DisableDoingTimer();
    EnableIdleTimer();
    // 尝试处理下一个请求
    TryNextAction();

    // 告诉上层请求处理结束
    action->HandleProcess(buf, len);

}

void CNetHandler::TryNextAction()
{
    if( _conn_type == ConnType_Short
            || _conn_type == ConnType_Long)
    {
        return;
    }


    int64_t now = __spp_get_now_ms();
    while (!_pending_action_list.empty())
    {
        SActionNode* actionNode = *_pending_action_list.begin();
        int timeout = 0;
        actionNode->actionInfo->GetTimeout(timeout);

        // 当前action 已经超时
        if (actionNode->en_time + (int64_t)timeout <= now)
        {
            CActionInfo* actionInfo = actionNode->actionInfo;
            //设置actioninfo执行时间为在pendinglist 上呆的时间
            actionInfo->SetTimeCost(now - actionNode->en_time);
            FramePtr->FRAME_LOG( LOG_ERROR, "%p - (%s, %u, %s): pending cost time: %lld(ms), timeout: %lld(ms), Msg: %p\n",
                                                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), now - actionNode->en_time, timeout, MsgPtr );
            actionInfo->HandleError(ETimeout);
            delete actionNode;
            _pending_action_list.pop_front();

        }
        else
        {
            break;
        }
    }
    if ( _pending_action_list.empty() )
    {
        return;
    }
    // 获取下一个待处理的请求
    SActionNode* actionNode = *_pending_action_list.begin();
    _action = actionNode->actionInfo;
    delete actionNode;
    _pending_action_list.pop_front();



    // 启动请求处理
    StartActionProcess();

}

int CNetHandler::StartActionProcess()
{
    FramePtr->FRAME_LOG( LOG_TRACE, "%p - (%s, %u, %s): Msg: %p, conntype: %s\n",
                            this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto),
                            MsgPtr, ConnTypeToStr(_conn_type) );

    _proc_state = SENDING;
    _have_sent_len = 0;
    _have_recv_len = 0;

    int timeout = 0;
    _action->GetTimeout( timeout );

    // 设置请求超时定时器
    /*
    _timerList = _timerUnit->GetTimerList(timeout);
    if(_timerList != NULL)
    {
        AttachTimer( _timerList );
    }
    */
    DisableIdleTimer();
    EnableDoingTimer(timeout);

    __spp_get_now_ms();
    _start_tv = __spp_g_now_tv;

    if( UNCONNECTED == _conn_state)
    {// 如果当前连接还未连接成功，则创建连接
        return Open();
    }

    return DoSend();
}


void CNetHandler::UpdateRouteResultIfNeed(int ecode)
{
    if( ecode == 0 ||
            (ecode != EConnect
            && ecode != ESend
            && ecode != ERecv
            && ecode != ECheckPkg
            && ecode != ETimeout
            && ecode != EHangup
            && ecode != EShutdown))
    {
        ecode = 0;
    }


    int modid = 0, cmdid = 0;
    std::string host_ip;
    unsigned short host_port = 0;
    _action->GetDestIp( host_ip );
    _action->GetDestPort( host_port );
    int cost = 0;
    _action->GetTimeCost(cost);
    if( cost < 1 ) {
        cost = 1;
    }

    // add 2.10.8 release2, for L5 api report
    if (g_spp_L5us != 0) {
        cost *= 1000;    
    }

    RouteType route_type = _action->GetRouteType();
    switch( route_type )
    {
        case L5NonStateRoute:
            {
                CNonStateActionInfo *pActionInfo = (CNonStateActionInfo *)_action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        case L5StateRoute:
            {
                CStateActionInfo *pActionInfo = (CStateActionInfo *)_action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        case L5AntiParalRoute:
            {
                CAntiParalActionInfo *pActionInfo = (CAntiParalActionInfo*)_action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        default:
            break;
    }


}
void CNetHandler::DisableIdleTimer()
{
    DisableTimer();
}
void CNetHandler::EnableIdleTimer()
{
    if (_timerListIdle != NULL)
    {
        AttachTimer(_timerListIdle);
    }
    return;
}
void CNetHandler::EnableDoingTimer(int timeout)
{
    _timerList = _timerUnit->GetTimerList(timeout);
    if(_timerList != NULL)
    {
        AttachTimer( _timerList );
    }
}
void CNetHandler::DisableDoingTimer()
{
    DisableTimer();
}
void CNetHandler::SetCanRecycle()
{
    _proc_state = IDLE_RECYCLE;
}
bool CNetHandler::CanRecycle()
{
    return _proc_state == IDLE_RECYCLE ? true : false;
}
void CNetHandler::Recycle()
{
    FramePtr->FRAME_LOG( LOG_DEBUG,  "%p - (%s, %u, %s): recycle, Msg: %p\n",
                    this, _dest_ip.c_str(), _dest_port, ProtoTypeToStr(_proto), MsgPtr);
    Close();
}
