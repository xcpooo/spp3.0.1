/*
 * =====================================================================================
 *
 *       Filename:  StateActionInfo.cpp
 *
 *    Description:  L5有状态路由Action描述对象
 *
 *        Version:  1.0
 *        Created:  11/16/2010 11:15:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "StateActionInfo.h"

USING_ASYNCFRAME_NS;

CStateActionInfo::CStateActionInfo(int init_buf_size)
    : CActionInfo( init_buf_size )
    , _modid(-1), _key(0), _funid(0), _cmdid(-1)
{
    _route_type = L5StateRoute;
}

CStateActionInfo::~CStateActionInfo()
{
}

void CStateActionInfo::SetRouteKey(int modid, int64_t key, int32_t funid)
{
    _modid = modid;
    _key = key;
    _funid = funid;
}

void CStateActionInfo::GetRouteKey(int &modid, int64_t &key, int32_t &funid)
{
    modid = _modid;
    key = _key;
    funid = _funid;
}

void CStateActionInfo::SetCmdID( int cmdid )
{
    _cmdid = cmdid;
}

void CStateActionInfo::GetRouteID( int &modid, int &cmdid )
{
    modid = _modid;
    cmdid = _cmdid;
}
