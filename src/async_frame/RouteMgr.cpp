/**
 * @file RouteMgr.cpp
 * @brief L5路由管理者
 * @author aoxu, aoxu@tencent.com
 * @version 1.1
 * @date 2011-10-09
 */
#include "RouteMgr.h"
#include "ActionSet.h"
#include "AsyncFrame.h"
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
#include "qos_client.h"

#define C_DEFAULT_MAX_NUM 3


using namespace L5API;
USING_L5MODULE_NS;

#define ROUTE_GET_ROUTE_ERR_TYPE(err__)  (err__ == L5API::QOS_RTN_OVERLOAD ? ROUTE_ERROR_OVERLOAD : ROUTE_ERROR_GET_FAILED)


CRouteMgr * CRouteMgr::_s_instance = NULL;

CRouteMgr* CRouteMgr::Instance (void)
{
    if( NULL == _s_instance )
    {
        _s_instance =  new CRouteMgr;
    }

    return _s_instance;
}

void CRouteMgr::Destroy (void)
{
    if( _s_instance != NULL )
    {
        delete _s_instance;
        _s_instance = NULL;
    }
}

CRouteMgr::CRouteMgr()
    : _asyncFrame(NULL), _onRouteCompleted(NULL)
{
}

CRouteMgr::~CRouteMgr()
{
}


// Set frame info and route complete callback func
void CRouteMgr::Init(CAsyncFrame* asyncFrame, RouteCompletedFunc cbFunc)
{
    _asyncFrame = asyncFrame;
    _onRouteCompleted = cbFunc;
}


// Get nonstate route from L5 async api
int CRouteMgr::HandleL5NonStateRoute(CNonStateActionInfo *pAI)
{
    // 1. get modid cmdid
    int32_t modid = -1, cmdid = -1;
    pAI->GetRouteID(modid, cmdid);
    if (modid <= 0 || cmdid <= 0) 
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Modid or cmdid is zero! modid: %d, cmdid: %d.\n", modid, cmdid);
        return _onRouteCompleted(_asyncFrame, pAI, ROUTE_ERROR_KEYID_INVALID, modid, cmdid);
    }
    _asyncFrame->FRAME_LOG(LOG_TRACE, "Begin to Get Route, modid: %d, cmdid: %d.\n", modid, cmdid);

    // 2. async get route from L5
    std::string err_msg;
    QOSREQUEST qos_req;
    qos_req._modid  = modid;
    qos_req._cmd    = cmdid;

    int32_t ret = AsyncApiGetRoute(qos_req, err_msg);
    if (ret < 0)
    {
        _asyncFrame->FRAME_LOG(LOG_ERROR, "Modid: %d, cmdid: %d get failed, ret %d[%s]\n", 
				modid, cmdid, ret, err_msg.c_str());
        return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_ROUTE_ERR_TYPE(ret), modid, cmdid);
    }

    // 3. success return
    pAI->SetDestIp(qos_req._host_ip.c_str());
    pAI->SetDestPort(qos_req._host_port);

    _asyncFrame->FRAME_LOG(LOG_TRACE, "Get Route success, modid: %d, cmdid: %d, ip: %s:%d\n",
            modid, cmdid, qos_req._host_ip.c_str(), qos_req._host_port);

    return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_SUCCESS, modid, cmdid);;
}

// Get state route from L5 async api
int CRouteMgr::HandleL5StateRoute(CStateActionInfo *pAI)
{
    // 1. get modid key funid
    int32_t modid = -1, cmdid = -1;
    int64_t key = 0;
    int32_t funid = 0;
    pAI->GetRouteKey(modid, key, funid);

    _asyncFrame->FRAME_LOG( LOG_TRACE, "Begin to Get Route, modid: %d, key: %lld, funid: %d\n", modid, key, funid);

    // 2. async get route from L5
    std::string err_msg;
    QOSREQUEST_MTTCEXT_HASH qos_req;
    qos_req._modid  = modid;
    qos_req._key    = key;
    qos_req._funid  = funid;

    int32_t ret = AsyncApiGetRoute(qos_req, err_msg);
    if (ret < 0)
    {
        _asyncFrame->FRAME_LOG(LOG_ERROR, "Modid: %d, key: %lld, funid: %d, get failed, ret %d[%s]\n", 
                    modid, key, funid, ret, err_msg.c_str());
        return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_ROUTE_ERR_TYPE(ret), modid, -1);
    }

    // 3. success return
    cmdid = qos_req._cmd;
    pAI->SetCmdID( cmdid );
    pAI->SetDestIp(qos_req._host_ip.c_str());
    pAI->SetDestPort(qos_req._host_port);

    _asyncFrame->FRAME_LOG(LOG_TRACE, "Get Route success, modid: %d, cmdid: %d, ip: %s:%d\n",
            modid, cmdid, qos_req._host_ip.c_str(), qos_req._host_port);

    return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_SUCCESS, modid, cmdid);;
}

// Get anti paral route from L5 async api
int CRouteMgr::HandleL5AntiParalRoute( CAntiParalActionInfo *pAI )
{
    // 1. get modid cmdid key
    int32_t modid = -1, cmdid = -1;
    int64_t key = 0;

    pAI->GetRouteKey( modid, cmdid, key );
    _asyncFrame->FRAME_LOG(LOG_TRACE, "Begin to Get Route, modid: %d, cmdid: %d, key: %lld.\n", modid, cmdid, key);

    // 2. async get route from L5
    std::string err_msg;
    QOSREQUEST_MTTCEXT qos_req;
    qos_req._modid  = modid;
    qos_req._cmdid  = cmdid;
    qos_req._key    = key;

    int32_t ret = ApiAntiParallelGetRoute(qos_req, err_msg);
    if (ret < 0)
    {
        _asyncFrame->FRAME_LOG(LOG_ERROR, "Modid: %d, cmdid: %d get failed, ret %d[%s]\n",
		 modid, cmdid, ret, err_msg.c_str());
        return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_ROUTE_ERR_TYPE(ret), modid, cmdid);
    }

    // 3. success return
    pAI->SetDestIp(qos_req._host_ip.c_str());
    pAI->SetDestPort(qos_req._host_port);
    pAI->SetIdealHost(qos_req._host_ip, qos_req._host_port);

    _asyncFrame->FRAME_LOG(LOG_TRACE, "Get Route success, modid: %d, cmdid: %d, ip: %s:%d\n",
            modid, cmdid, qos_req._host_ip.c_str(), qos_req._host_port);

    return _onRouteCompleted(_asyncFrame, pAI, ROUTE_GET_SUCCESS, modid, cmdid);;
}


int CRouteMgr::UpdateRouteResult(int modid,
        int cmdid,
        const std::string &ip,
        unsigned short port,
        int err,
        int delay)
{
    _asyncFrame->FRAME_LOG( LOG_TRACE, "Update Route Result, modid: %d, cmdid: %d, ip: %s, port: %d, errno: %d, delay: %d\n",
                                    modid, cmdid, ip.c_str(), port, err, delay);
    QOSREQUEST qos_req;
    qos_req._modid      = modid;
    qos_req._cmd        = cmdid;
    qos_req._host_ip    = ip;
    qos_req._host_port  = port;
    AsyncApiRouteResultUpdate(qos_req, err, TIME_MILLISECOND, delay);
    return 0;
}



void CRouteMgr::RouteKey2RouteID(uint64_t key, int &modid, int &cmdid)
{
    modid = (int)(key >> 32);
    cmdid = (int)(key & 0xFFFFFFFF);
}

void CRouteMgr::RouteID2RouteKey(int modid, int cmdid, uint64_t &key )
{
    key = modid;
    key = (key << 32) | cmdid;
}



void CRouteMgr::InitConnNumLimit(int modid, int cmdid, unsigned limit)
{
    if(modid < 1)
    {
        return;
    }

    if(cmdid < 1)
    {
        cmdid = -1;
    }

    if(0 == limit)
    {
        limit = C_DEFAULT_MAX_NUM;
    }

    uint64_t route_key = (uint64_t)modid<<32|cmdid;
    std::map<uint64_t, unsigned>::iterator it = _routeLimitMap.find( route_key );
    if(it == _routeLimitMap.end())
    {
        _routeLimitMap.insert( RouteLimitMap::value_type( route_key, limit) );
    } // else 存在，什么也不做
}

unsigned CRouteMgr::GetConnNumLimit(int modid, int cmdid)
{
    if(modid < 1)
    {
        return 0;
    }

    // 有状态路由(cmdid == -1)同样存储到_routeLimitMap里
    if(cmdid < 1)
    {
        cmdid = -1;
    }

    uint64_t route_key = (uint64_t)modid<<32|cmdid;
    std::map<uint64_t, unsigned>::iterator it = _routeLimitMap.find( route_key );
    if(it == _routeLimitMap.end())
    {
        return 0;
    }
    else
    {
        return it->second;
    }
}


