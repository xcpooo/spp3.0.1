/*
 * =====================================================================================
 *
 *       Filename:  StaticRoute.cpp
 *
 *    Description:  静态路由，当l5agent进程不存在时触发
 *
 *        Version:  1.0
 *        Created:  11/09/2010 10:50:45 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "StaticRoute.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "atomic_int64.h"
#include "timestamp.h"

//USING_L5MODULE_NS;
BEGIN_L5MODULE_NS

int compare_route (const void* p1, const void* p2)
{
    const CStaticRoute::route_item &item1 = *(const CStaticRoute::route_item *)p1;
    const CStaticRoute::route_item &item2 = *(const CStaticRoute::route_item *)p2;
    if( item1._ip < item2._ip )
    {
        return -1;
    }
    else if( item1._ip > item2._ip )
    {
        return 1;
    }

    if( item1._port < item2._port )
    {
        return -1;
    }
    else if( item1._port > item2._port )
    {
        return 1;
    }

    return 0;
}



CStaticRoute::CStaticRoute(const char *hb_map_file, const char *route_table_file)
    : _asyncFrame(NULL), _hbMap(NULL), _routeFileMtime(0)
{
    _hbMapFile = hb_map_file;
    _routeTableFile = route_table_file;

    _hbMap = CHBMap::InitMap(_hbMapFile.c_str());
}

CStaticRoute::~CStaticRoute()
{
    if( _hbMap != NULL )
    {
        CHBMap::FiniMap(_hbMap);
        _hbMap = NULL;
    }
}

void CStaticRoute::Init(CAsyncFrame* asyncFrame)
{
    _asyncFrame = asyncFrame;
}

bool CStaticRoute::IsActive()
{
   uint64_t cur_tm = __spp_get_now_s();

   uint64_t hb_tm = __mmx_readq(&(_hbMap->_hb_tm));
   
   if( cur_tm >= hb_tm
           && (cur_tm - hb_tm) > HEARTBEAT_UPDATE_DELAY )
   {
       return false;
   }

   return true;
}

int CStaticRoute::ReloadRouteTable()
{
    int ret = 0;
    FILE* pf=fopen( _routeTableFile.c_str(),"r");
    if(NULL==pf)
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Open Route Table File Failed: %s\n", _routeTableFile.c_str());
        return RouteFileOpenFail;
    }

    int num = 0;
    do
    {
        int modid, cmdid, port;
        char ip[1024];
        num = fscanf( pf, "%d %d %s %d\n", &modid, &cmdid, ip, &port );
        if( num == 4 )
        {
            struct in_addr in;
            if(inet_aton(ip,&in)==0)
            {
                ret = InvalidRouteIP;
                break;
            }
                    
            AddRoute( modid, cmdid, in.s_addr, port );
        }
    }while( num != EOF );

    fclose( pf );

    SortRouteTable();

    return ret;
}

void CStaticRoute::SortRouteTable()
{
    std::map<uint64_t, static_route_t>::iterator it = _routeMap.begin();
    for( ; it != _routeMap.end(); it++ )
    {
        it->second.SortRoute();
    }
}

int CStaticRoute::IsSwitchToStatic()
{
    if( NULL == _hbMap )
    {
        _hbMap = CHBMap::InitMap(_hbMapFile.c_str());
        if( NULL == _hbMap )
        {// L5Agent不支持Heartbeat功能
            return NotSuportStaticRoute;
        }
    }

    if( IsActive() )
    {
        return L5AgentIsActive;
    }

    return SwitchToStaticRoute;

}

int CStaticRoute::ReloadRouteTableIfNeed()
{
    struct stat statbuf;
    if(stat(_routeTableFile.c_str(), &statbuf))
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Stat Route Table File Failed: %s\n", _routeTableFile.c_str());
        return RouteFileStatFail;
    }   

    if( statbuf.st_mtime != _routeFileMtime )
    {
        _routeFileMtime = statbuf.st_mtime;

        _asyncFrame->FRAME_LOG( LOG_NORMAL, "Reload Static RouteTable: %s\n", _routeTableFile.c_str());

        int ret = ReloadRouteTable();
        if( ret != 0 )
        {
            return ret;
        }
    }

    return ReloadRouteTableSuccess;
}

int CStaticRoute::GetRoute(int modid, 
        int cmdid, 
        std::string& ip,
        unsigned short& port)
{
    uint64_t route_key = (uint64_t)modid<<32|cmdid; 
    std::map<uint64_t,static_route_t>::iterator it = _routeMap.find( route_key );
    if( it == _routeMap.end() )
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Find no Route, modid: %d, cmdid: %d\n", modid, cmdid);
        return NoRouteKey;
    }
    int ip_val ;
    if( !it->second.GetRoute(ip_val,port) )
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Empty Route, modid: %d, cmdid: %d\n", modid, cmdid);
        return NoRoute;
    }

    struct in_addr in;
    in.s_addr = ip_val;
    ip = (char*)inet_ntoa(in);

    return GetRouteSuccess;
}

int CStaticRoute::GetRoute(int modid,
        int cmdid,
        int64_t key,
        int &ip,
        unsigned short &port)
{
    uint64_t route_key = (uint64_t)modid<<32|cmdid;
    std::map<uint64_t,static_route_t>::iterator it = _routeMap.find( route_key );
    if( it == _routeMap.end() )
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Find no Route, modid: %d, cmdid: %d\n", modid, cmdid);
        return NoRouteKey;
    }

    if( !it->second.GetRoute( key, ip, port ) )
    {
        _asyncFrame->FRAME_LOG( LOG_ERROR, "Empty Route, modid: %d, cmdid: %d\n", modid, cmdid);
        return NoRoute;
    }

    return GetRouteSuccess;
}

void CStaticRoute::AddRoute(int modid, int cmdid, int ip, unsigned short port)
{
    static_route_t route(modid,cmdid);

    std::pair< std::map<uint64_t,static_route_t>::iterator,bool> insert_ret=
        _routeMap.insert( std::map<uint64_t,static_route_t>::value_type(route._route_key,route) );

    insert_ret.first->second.AddRoute(ip,port);
}

END_L5MODULE_NS
