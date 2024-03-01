/*
 * =====================================================================================
 *
 *       Filename:  NonStateActionInfo.cpp
 *
 *    Description:  无状态L5路由的Action Info
 *
 *        Version:  1.0
 *        Created:  11/11/2010 03:46:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "NonStateActionInfo.h"

USING_ASYNCFRAME_NS;

CNonStateActionInfo::CNonStateActionInfo(int init_buf_size)
    : CActionInfo( init_buf_size ), _modid(-1), _cmdid(-1)
{
    _route_type = L5NonStateRoute;
}

CNonStateActionInfo::~CNonStateActionInfo()
{
}


void CNonStateActionInfo::SetRouteID(int modid, int cmdid)
{
    _modid = modid;
    _cmdid = cmdid;
}

void CNonStateActionInfo::GetRouteID(int &modid, int &cmdid)
{
    modid = _modid;
    cmdid = _cmdid;
}



