/*
 * =====================================================================================
 *
 *       Filename:  MsgBase.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/27/2010 10:20:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "MsgBase.h"
#include "IFrameStat.h"
#include "StatComDef.h"
#include <sys/time.h>
#include "timestamp.h"

USING_ASYNCFRAME_NS;
extern struct timeval __spp_g_now_tv;

CMsgBase::CMsgBase()
    : _srvbase(NULL), _commu(NULL), _flow(-1), _info_flag(false), _msg_timeout(0)
{
	__spp_get_now_ms();
    _start_tv = __spp_g_now_tv;
}

CMsgBase::~CMsgBase()
{
    STEP_REQ_STAT(GetMsgCost());
}
void CMsgBase::AppendInfo(const char *str)
{
    if( !_info_flag )
    {
        return ;
    }
    _info << str;
}

int CMsgBase::SendToClient(blob_type &blob)
{
    if( NULL == _commu ) {
        return -999;
    }

    int ret = _commu->sendto( _flow, &blob, _srvbase);
    return ret;
}

void CMsgBase::SetMsgTimeout(int timeout)
{
    _msg_timeout = timeout;

    if( _msg_timeout < 0 ) {
        _msg_timeout = 0;
    }
}


int CMsgBase::GetMsgTimeout()
{
    return _msg_timeout;
}

bool CMsgBase::CheckMsgTimeout()
{
    if( _msg_timeout <= 0 )
    {// 无需检查请求处理超时
        return false;
    }

    int cost = GetMsgCost();
    if( cost < _msg_timeout )
    {// 未超时
        return false;
    }

    // 超时了
    return true;
}

int CMsgBase::GetMsgCost()
{
    // 计算请求处理的时间开销
	__spp_get_now_ms();    
    struct timeval tv = __spp_g_now_tv;

    //printf("%lld-%lld:%lld-%lld\n", _start_tv.tv_sec,  _start_tv.tv_usec, tv.tv_sec, tv.tv_usec);

    int cost = (tv.tv_sec - _start_tv.tv_sec) * 1000;

    if( tv.tv_usec > _start_tv.tv_usec )
    {
        cost += (tv.tv_usec-_start_tv.tv_usec) / 1000;
    }
    else
    {
        cost -= (_start_tv.tv_usec-tv.tv_usec) / 1000;
    }

    if( cost < 0 ) 
    {
        cost = 0;
    }

    return cost;

}
