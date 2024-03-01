/*
 * =====================================================================================
 *
 *       Filename:  AntiParalActionInfo.cpp
 *
 *    Description:  防并发路由机制的Action Info
 *
 *        Version:  1.0
 *        Created:  11/19/2010 11:42:34 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "AntiParalActionInfo.h"

USING_ASYNCFRAME_NS;

 CAntiParalActionInfo::CAntiParalActionInfo(int init_buf_size)
    : CActionInfo( init_buf_size )
    , _modid(-1), _key(0), _cmdid(-1)
{
    _route_type = L5AntiParalRoute;
}

CAntiParalActionInfo::~CAntiParalActionInfo()
{
}

void CAntiParalActionInfo::SetRouteKey(int modid, int cmdid, int64_t key)
{
    _modid = modid;
    _cmdid = cmdid;
    _key = key;
}

void CAntiParalActionInfo::GetRouteKey(int &modid, int &cmdid, int64_t &key)
{
    modid = _modid;
    cmdid = _cmdid;
    key = _key;
}

void CAntiParalActionInfo::GetRouteID( int &modid, int &cmdid )
{
    modid = _modid;
    cmdid = _cmdid;
}


void CAntiParalActionInfo::SetIdealHost( const std::string &ideal_ip, PortType ideal_port )
{
    _ideal_ip = ideal_ip;
    _ideal_port = ideal_port;
}

void CAntiParalActionInfo::GetIdealHost( std::string &ideal_ip, PortType &ideal_port )
{
    ideal_ip = _ideal_ip;
    ideal_port = _ideal_port;
}

