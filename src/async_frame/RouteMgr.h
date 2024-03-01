/**
 * @file RouteMgr.h
 * @brief L5路由管理者
 * @author aoxu, aoxu@tencent.com
 * @version 1.1
 * @date 2011-10-09
 */
#ifndef __ROUTE_MGR_H__
#define __ROUTE_MGR_H__

#include <stdint.h>
#include <string>
#include <map>
#include "CommDef.h"
#include "NonStateActionInfo.h"
#include "StateActionInfo.h"
#include "AntiParalActionInfo.h"
#include <poller.h>
#include <timerlist.h>

#define BEGIN_L5MODULE_NS namespace SPP_L5ROUTE{
#define END_L5MODULE_NS }
#define USING_L5MODULE_NS using namespace SPP_L5ROUTE

USING_ASYNCFRAME_NS;

BEGIN_L5MODULE_NS

// Route Errno Define
#define ROUTE_GET_SUCCESS                    0
#define ROUTE_ERROR_KEYID_INVALID           -1
#define ROUTE_ERROR_KEY_INIT_FAILED         -2
#define ROUTE_ERROR_GET_FAILED              -3
#define ROUTE_ERROR_OVERLOAD                -4

/**
 * @brief Route manager class
 */
class CRouteMgr
{
    public:
        static CRouteMgr* Instance (void);
        static void Destroy (void);

        typedef int (*RouteCompletedFunc)(CAsyncFrame*, CActionInfo*, int ecode, int modid, int cmdid);

        /**
         * @brief Set async frame info
         * @param asyncFrame - frame info
         */
        void Init(CAsyncFrame* asyncFrame, RouteCompletedFunc cbFunc);

        /**
         * 处理L5无状态路由请求
         *
         * @param pAI CNonStateActionInfo对象指针
         *
         */
        int HandleL5NonStateRoute( CNonStateActionInfo *pAI );

        /**
         * 处理L5有状态路由请求
         *
         * @param pAI CStateActionInfo对象指针
         *
         */
        int HandleL5StateRoute( CStateActionInfo *pAI );

        /**
         * 处理L5防并发路由请求
         *
         * @param pAI CAntiParalActionInfo对象指针
         *
         */
        int HandleL5AntiParalRoute( CAntiParalActionInfo *pAI );

        /**
         * 上报路由执行结果
         */
        int UpdateRouteResult(int32_t modid, 
                                int32_t cmdid,
                                const std::string &ip,
                                unsigned short port,
                                int result,
                                int delay);


        // L5根据一组modid/cmdid来管理连接数上限，对应的每组ip/port都是最多limit个连接
        /**
         * @brief 设置一组modid/cmdid的连接数上限
         *
         * @param modid 必填参数
         * @param cmdid 仅对使用有状态路由时填-1
         * @param limit 连接数上限
         */
        void InitConnNumLimit(int modid, int cmdid, unsigned limit);
        
         /**
          * @brief 取一组modid/cmdid的连接数上限
          * @param modid 必填参数
          * @param cmdid 仅对使用有状态路由时填-1
          * @return 当前连接数上限
          */
        unsigned GetConnNumLimit(int modid, int cmdid);
        

    private:
    
        CRouteMgr();
        ~CRouteMgr();

        void RouteKey2RouteID(uint64_t key, int &modid, int &cmdid);
        void RouteID2RouteKey(int modid, int cmdid, uint64_t &key );


    private:
    
        typedef std::map<uint64_t, unsigned> RouteLimitMap;
        
        static CRouteMgr * _s_instance;

        CAsyncFrame *_asyncFrame;
        RouteCompletedFunc _onRouteCompleted;

        std::map<uint64_t, unsigned> _routeLimitMap; // 路由连接数上限
};

END_L5MODULE_NS

#endif
