/*
 * =====================================================================================
 *
 *       Filename:  StaticRoute.h
 *
 *    Description:  静态路由，当l5agent进程不存在时触发
 *
 *        Version:  1.0
 *        Created:  11/09/2010 10:51:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __STATIC_ROUTE_H__
#define __STATIC_ROUTE_H__

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "L5Def.h"
#include "L5HbMap.h"
#include "AsyncFrame.h"

BEGIN_L5MODULE_NS

USING_ASYNCFRAME_NS;


int compare_route (const void* p1, const void* p2);

class CStaticRoute
{
    public:
        typedef enum 
        {
            SwitchToStaticRoute = 0,
            NotSuportStaticRoute = -1,
            L5AgentIsActive = -2,

            ReloadRouteTableSuccess = 0,
            RouteFileStatFail = -1,
            RouteFileOpenFail = -2,
            InvalidRouteIP = -3,

            GetRouteSuccess = 0,
            NoRouteKey = -1,
            NoRoute = -2,


        }RetCode;

        typedef struct route_item
        {
            route_item(int ip, unsigned short port)
                :_ip(ip),_port(port)
            {
            }

            bool operator<(const route_item& i)
            {
                if( _ip < i._ip ){
                    return true;
                } else if( _ip > i._ip ){
                    return false;
                }
                return (_port < i._port) ? true : false;
            }

            bool operator==(const route_item& i)
            {
                return (_ip==i._ip)&&(_port==i._port);
            }
            int _ip;
            unsigned short _port;
        }route_item_t;

        typedef struct static_route
        {
            static_route(int mod, int cmd)
                : _route_key((uint64_t)mod<<32|cmd)
                , _poll_index(0)
            {
            }

            void AddRoute(int ip, int port)
            {
                route_item_t item(ip, port);
                std::vector<route_item_t>::const_iterator it;

                it = std::find(_route_item_vec.begin(),_route_item_vec.end(),item);
                if(it == _route_item_vec.end())
                {
                    _route_item_vec.push_back(item);
                }
            }

            void SortRoute()
            {
                if( _route_item_vec.empty() )
                {
                    return;
                }
                qsort( &_route_item_vec[0], _route_item_vec.size(), sizeof(route_item_t), compare_route );
            }

            bool GetRoute(int &ip, unsigned short& port)
            {
                if(_route_item_vec.empty())
                {
                    return false;
                }

                ip = _route_item_vec[_poll_index]._ip;
                port = _route_item_vec[_poll_index]._port;

                if( ++_poll_index >= _route_item_vec.size())
                {
                    _poll_index=0;
                }

                return true;
            }

            bool GetRoute( uint64_t key, int &ip, unsigned short& port)
            {
                if(_route_item_vec.empty())
                {
                    return false;
                }

                uint32_t route_size = _route_item_vec.size();
                uint32_t idx = (uint32_t)(key % route_size);

                const route_item_t & item = _route_item_vec[idx];

                ip = item._ip;
                port = item._port;

                return true;

            }

            uint64_t        _route_key;
            std::vector<route_item_t> _route_item_vec;
            uint32_t        _poll_index;
        }static_route_t;

    public:
        CStaticRoute(const char *hb_map_file, const char *route_table_file);
        ~CStaticRoute();

        void Init(CAsyncFrame* asyncFrame);

        /**
         * 检测是否需要切换到静态路由
         *
         * @return SwitchToStaticRoute: 切换到静态路由, 其他：不需要
         */
        int IsSwitchToStatic();

        /**
         * 如果需要，重新加载RouteTable
         *
         */
        int ReloadRouteTableIfNeed();

        /**
         * 获取静态路由，平均分配原则
         *
         * @return GetRouteSuccess: 获取路由成功， 其他： 失败
         */
        int GetRoute(int modid, 
                        int cmdid, 
                        std::string& ip,
                        unsigned short& port);

        /**
         * 根据key进行路由选择, key%route_size原则
         *
         * @return GetRouteSuccess: 获取路由成功， 其他： 失败
         */
        int GetRoute(int modid, 
                        int cmdid, 
                        int64_t key, 
                        int &ip, 
                        unsigned short &port);

    private:
        bool IsActive();


        int ReloadRouteTable();
        void SortRouteTable();

        void AddRoute(int modid, int cmdid, int ip, unsigned short port);

    private:
        CAsyncFrame* _asyncFrame;

        std::string _hbMapFile;
        std::string _routeTableFile;

        hb_map_t * _hbMap;
        time_t _routeFileMtime;

        std::map<uint64_t, static_route_t> _routeMap;
};


END_L5MODULE_NS

#endif
