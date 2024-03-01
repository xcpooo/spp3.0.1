/*
 * =====================================================================================
 *
 *       Filename:  ActionSet.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/13/2010 05:35:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "ActionSet.h"
#include "AsyncFrame.h"
#include "ActionInfo.h"
#include "NetMgr.h"
#include "NetHandler.h"
#include "MsgBase.h"
#include "RouteMgr.h"

USING_ASYNCFRAME_NS;
USING_L5MODULE_NS;

CActionSet::CActionSet(CAsyncFrame *pFrame, 
        IState *pState,
        CMsgBase *pMsg)
    : _pFrame(pFrame), _pState(pState)
    , _pMsg(pMsg), _finished_count(0)
{
}

CActionSet::~CActionSet()
{
    ClearAction();
}

int CActionSet::AddAction( CActionInfo *ai )
{
    if( NULL == ai ) {
        return -1;
    }
    
    if( _ActionSet.find( ai ) != _ActionSet.end() ) 
    { // 已经存在 
        return 0;
    }

    ai->SetFramePtr( _pFrame );
    ai->SetActionSetPtr( this );
    ai->SetMsgPtr( _pMsg );

    _ActionSet.insert( ai );

    return 0;
}


void CActionSet::ClearAction()
{
    ActionSet::iterator it = _ActionSet.begin();
    for( ; it != _ActionSet.end(); it++ )
    {
        delete *it;
    }

    _ActionSet.clear();
}

int CActionSet::StartAction()
{
    bool bTimeout = _pMsg->CheckMsgTimeout();

    int ret = 0;
    size_t size = _ActionSet.size();
    size_t count = 0;

    ActionSet::iterator it = _ActionSet.begin();
    for(; it != _ActionSet.end(); it++ )
    {
        CActionInfo *ai = *it;
        count++;
 
        if( bTimeout )
        {// 请求处理总体超时
            ai->HandleError( EMsgTimeout );

            if( count == size ){
                // 当最后一个Action处理失败时，应该马上返回,
                //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                return 0;
            }

            continue;
        }

        int route_type = ai->GetRouteType();
        switch( route_type )
        {
            case NoGetRoute:
                {
                    ret = ai->HandleStart();

                    if( ret < 0
                            && count == size )
                    {
                        // 当最后一个Action处理失败时，应该马上返回,
                        //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                        return 0;
                    }

                    break;
                }
            case L5NonStateRoute:
                {
                    ret = CRouteMgr::Instance()->HandleL5NonStateRoute( (CNonStateActionInfo *)ai );

                    if( ret < 0
                            && count == size )
                    {
                        // 当最后一个Action处理失败时，应该马上返回,
                        //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                        return 0;
                    }

                    break;
                }
            case L5StateRoute:
                {
                    ret = CRouteMgr::Instance()->HandleL5StateRoute( (CStateActionInfo *)ai );

                    if( ret < 0
                            && count == size )
                    {
                        // 当最后一个Action处理失败时，应该马上返回,
                        //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                        return 0;
                    }

                    break;
                }
            case L5AntiParalRoute:
                {
                    ret = CRouteMgr::Instance()->HandleL5AntiParalRoute( (CAntiParalActionInfo*)ai );

                    if( ret < 0
                            && count == size )
                    {
                        // 当最后一个Action处理失败时，应该马上返回,
                        //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                        return 0;
                    }

                    break;
                }
            default:
                {
                    ai->HandleError( EInvalidRouteType );

                    if( count == size ){
                        // 当最后一个Action处理失败时，应该马上返回,
                        //因为在CActionInfo::HandleError里已经销毁了CActionSet对象
                        return 0;
                    }

                    break;
                }
        }


    }

    return 0;
}

int CActionSet::HandleFinished( CActionInfo * ai )
{
    CNetHandler* pHandler = ai->GetNetHandler();
    if( pHandler != NULL )
    {
        CNetMgr::Instance()->RecycleHandler( pHandler );
    }

    _finished_count++;
    if( (int)_ActionSet.size() == _finished_count )
    {
        _pFrame->FRAME_LOG(LOG_TRACE, "Msg: %p, FinishedCnt: %d\n",
                                    _pMsg, _finished_count);
        _pFrame->HandleProcess(_pState, this, _pMsg);
    }

    return 0;
}


IState *CActionSet::GetIState()
{
    return _pState;
}

CMsgBase *CActionSet::GetMsg()
{
    return _pMsg;
}

