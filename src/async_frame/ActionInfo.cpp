/*
 * =====================================================================================
 *
 *       Filename:  ActionInfo.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/13/2010 03:40:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "ActionInfo.h"
#include "ActionSet.h"
#include "IAction.h"
#include "AsyncFrame.h"
#include "MsgBase.h"
#include "NetMgr.h"
#include "NetHandler.h"
#include "IFrameStat.h"
#include "StatComDef.h"
#include "misc.h"
#include "../comm/monitor.h"

// SEXInterface.cpp

BEGIN_ASYNCFRAME_NS
extern void SetCurrentActionInfo(void* ptr);
END_ASYNCFRAME_NS

USING_ASYNCFRAME_NS;
using namespace spp::comm;

#define BUFFER_PIECE_VAL 32

CActionInfo::CActionInfo(int init_buf_size)
    : _route_type(NoGetRoute), _pActionSet(NULL), _pAction(NULL), _pFrame(NULL)
    , _pMsg(NULL), _pHandler(NULL), _port(0), _proto(ProtoType_Invalid)
    , _type(ActionType_Invalid), _id(-1), _timeout(100), _errno(0)
    , _time_cost(0), _buf(NULL), _buf_size(init_buf_size), _real_data_len(0)
{
    memset(_ip, 0, sizeof(_ip));

    if( _buf_size < MIN_BUF_SIZE ) {
        _buf_size = MIN_BUF_SIZE;
    }

    _buf = (char *)malloc(_buf_size*sizeof(char));
}

CActionInfo::~CActionInfo()
{
    if( _buf != NULL ) {
        free(_buf);
        _buf = NULL;
    }
}

int CActionInfo::HandleEncode()
{
    SetCurrentActionInfo(this);
    _pFrame->FRAME_LOG(LOG_TRACE, "ID: %d, HandleEncode - Msg: %p\n", _id, _pMsg);

    int ret = 0;

    int buf_len = _buf_size;
    ret = _pAction->HandleEncode(_pFrame, _buf, buf_len, _pMsg);
    if( ret > 0 )
    {
        int need_size = ret - _buf_size;
        if( need_size > 0 )
        {
            _pFrame->FRAME_LOG(LOG_TRACE, "ID: %d, ExtendBuffer: %d - Msg: %p\n", _id, ret, _pMsg);

            ExtendBuffer(ret);

            buf_len = _buf_size;
            ret = _pAction->HandleEncode(_pFrame, _buf, buf_len, _pMsg);

        }
        else
        {
            _pFrame->FRAME_LOG(LOG_ERROR, "ID: %d, Msg: %p; IAction::HandleEncode return invalid value: "
                                        "> 0 but < buffer size\n", _id, _pMsg);
            return -999;
        }

    }

    if ( ret == 0 )
    {
        _real_data_len = buf_len;
    }


    return ret;
}

int CActionInfo::HandleInput( const char *buf, int len)
{
    SetCurrentActionInfo(this);
    _pFrame->FRAME_LOG(LOG_TRACE, "ID: %d, HandleInput - Msg: %p\n", _id, _pMsg);

    int ret = _pAction->HandleInput(_pFrame, buf, len, _pMsg);
    if( ret > 0 )
    {
        _real_data_len = ret;
    }
    else if ( ret == 0 && len == _buf_size && buf == _buf )
    { // 还未接收到完整包，但是buffer已经用完。↑在ParallelNetHandler时使用外部buffer
        ExtendBuffer( 2*_buf_size );
    }

    return ret;
}

int CActionInfo::HandleProcess( const char *buf, int len)
{
    SetCurrentActionInfo(this);
    _pFrame->FRAME_LOG(LOG_TRACE, "ID: %d, HandleProcess - Msg: %p; BufSize: %d\n", _id, _pMsg, len);

    _errno = 0;
    STEP_CODE_STAT(0, spperrToStr(0) );

    bool output = _pMsg->GetInfoFlag();
    if( output )
    {
        char info[1024];
        snprintf( info, sizeof(info), "Action ID: %d, ip: %s, port: %u, proto: %s, type: %s, errno: %d, timecost: %d;",
                                _id, _ip, _port, ProtoTypeToStr(_proto), ActionTypeToStr(_type), _errno, _time_cost);

        _pMsg->AppendInfo(info);
    }

    if ( buf != _buf )
    {
        _real_data_len = len;
        ExtendBuffer( len );// may not really Extend
        memcpy( _buf, buf, len );
    }

    _pAction->HandleProcess(_pFrame, buf, len, _pMsg);

    return _pActionSet->HandleFinished(this);
}

int CActionInfo::HandleError( int err_no )
{
    SetCurrentActionInfo(this);
    _pFrame->FRAME_LOG(LOG_ERROR, "ID: %d, HandleError - Msg: %p, Errno: %d\n", _id, _pMsg, err_no);
    STEP_CODE_STAT(err_no > 0 ? err_no : -err_no, spperrToStr(err_no) );

    _errno = err_no;

    bool output = _pMsg->GetInfoFlag();
    if( output )
    {
        char info[1024];
        snprintf( info, sizeof(info), "Action ID: %d, ip: %s, port: %u, proto: %s, type: %s, errno: %d, timecost: %d;",
                                _id, _ip, _port, ProtoTypeToStr(_proto), ActionTypeToStr(_type), _errno, _time_cost);

        _pMsg->AppendInfo(info);
    }

    //monitor错误码上报
    if( err_no<0 && err_no >-18)
    {
		MONITOR(AsyncFrame_Error_Begin-1-err_no);
    }
    else
    {
   		MONITOR(AsyncFrame_Error_Others);
    }


    _pAction->HandleError( _pFrame, err_no, _pMsg );

    return _pActionSet->HandleFinished(this);
}

bool CActionInfo::SendOnly()
{
    return ( _type == ActionType_SendOnly
                   || _type == ActionType_SendOnly_KeepAlive
                   || _type == ActionType_SendOnly_KeepAliveWithPending);
}

int CActionInfo::HandleSendFinished()
{
    _pFrame->FRAME_LOG(LOG_TRACE, "ID: %d, HandleSendFinished - Msg: %p\n", _id, _pMsg);

    if ( SendOnly() )
    {// 无需接收响应包
        _errno = 0;

        bool output = _pMsg->GetInfoFlag();
        if( output )
        {
            char info[1024];
            snprintf( info, sizeof(info), "Action ID: %d, ip: %s, port: %u, proto: %s, type: %s, errno: %d, timecost: %d;",
                    _id, _ip, _port, ProtoTypeToStr(_proto), ActionTypeToStr(_type), _errno, _time_cost);

            _pMsg->AppendInfo(info);
        }

        _pActionSet->HandleFinished(this);

        return 0;
    }

    // 需要接收响应包
    return 1;
}

void CActionInfo::ResetBuffer()
{
    _real_data_len = 0;
}

int CActionInfo::ExtendBuffer( int need_size )
{
    if( _buf_size < need_size ) {
        // Align...
        need_size = ( need_size + BUFFER_PIECE_VAL - 1 ) & ~(BUFFER_PIECE_VAL - 1);        
        _buf = (char *)CMisc::realloc_safe(_buf, need_size);
        _buf_size = need_size;
    }

    return ((_buf != 0) ? 0 : -1);
}

void CActionInfo::GetBuffer( char **buf, int &size )
{
    *buf = _buf;
    size = _real_data_len;
}

int CActionInfo::HandleStart()
{
    // 请求包编码
    int ret = HandleEncode();
    if( ret != 0 )
    {
        HandleError( EEncodeFail );
        return EEncodeFail;
    }

    std::string ip;
    PortType port;
    ProtoType proto;
    ActionType type;
    ConnType conn_type = ConnType_Short;

    GetDestIp(ip);
    GetDestPort(port);
    GetProto(proto);

    GetActionType(type);
    
    _pFrame->FRAME_LOG(LOG_TRACE, "ip: %s, port: %u, proto: %s, "
            "action_type: %s\n", ip.c_str(), port,
            ProtoTypeToStr(proto), ActionTypeToStr(type) );

    switch(type)
    {
        case ActionType_SendRecv:
        case ActionType_SendOnly:
            {
                conn_type = ConnType_Short;
                break;
            }
        case ActionType_SendRecv_KeepAlive:
        case ActionType_SendOnly_KeepAlive:
            {
                conn_type = ConnType_Long;
                break;
            }
        case ActionType_SendRecv_KeepAliveWithPending:
        case ActionType_SendOnly_KeepAliveWithPending:
            {
                conn_type = ConnType_LongWithPending;
                break;
            }
        case ActionType_SendRecv_Parallel: // 多发多收类型
            {
                conn_type = ConnType_LongWithPending;
                CParallelNetHandler* pHandler = 
                    CNetMgr::Instance()->GetParallelHandler(ip, port, proto, conn_type);
                SetNetHandler(NULL);
                return pHandler->HandleRequest(this);
            }
        default:
            break;
    }

    CNetHandler *pHandler = CNetMgr::Instance()->GetHandler(ip, port, proto, conn_type);
    SetNetHandler(pHandler);
    return pHandler->HandleRequest(this);

}
