/*
 * =====================================================================================
 *
 *       Filename:  AsyncFrame.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2010 03:27:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "AsyncFrame.h"
#include "NetMgr.h"
#include "ActionSet.h"
#include "IState.h"
#include "MsgBase.h"
#include "RouteMgr.h"
#include "defaultworker.h"
#include "SEXInterface.h"
#include "asyn_api.h"
#include "spp_async_interface.h"


using namespace spp::worker;
USING_ASYNCFRAME_NS;
USING_L5MODULE_NS;
using namespace spp::comm;

int OnRouteCompleted(CAsyncFrame* pFrame, CActionInfo* pActionInfo, int ecode, int modid, int cmdid)
{
    if( ecode == 0 )
    {
        ActionType type;
        ProtoType proto;
        pActionInfo->GetActionType(type);
        pActionInfo->GetProto(proto);
       
        // 仅对多发多收类型的TCP连接才作限制
        if(ActionType_SendRecv_Parallel == type && proto == ProtoType_TCP)
        {
            std::string dest_ip;
            PortType dest_port;
            pActionInfo->GetDestIp(dest_ip);
            pActionInfo->GetDestPort(dest_port);
            
            InitConnNumLimit(dest_ip, dest_port, CRouteMgr::Instance()->GetConnNumLimit(modid, cmdid));
        }
        return pActionInfo->HandleStart();
    }
    else if (ecode == ROUTE_ERROR_OVERLOAD)
    {
        pActionInfo->HandleError( EGetRouteOverload );
        return -1;
    }
    else
    {
        pActionInfo->HandleError( EGetRouteFail );
        return -1;
    }

    return 0;
}

CAsyncFrame *CAsyncFrame::_s_instance = NULL;

CAsyncFrame* CAsyncFrame::Instance (void)
{
    if( NULL == _s_instance )
    {
        _s_instance = new CAsyncFrame;
    }

    return _s_instance;
}

void CAsyncFrame::Destroy (void)
{
    if( _s_instance != NULL )
    {
        delete _s_instance;
        _s_instance = NULL;
    }
}

CAsyncFrame::CAsyncFrame()
    : _pServBase(NULL), _asptr_mgr(NULL), _msgptr_mgr(NULL)
    , _nOverloadLimit(0), _nRequestCount(0), _pNeedDeleteAS(NULL)
{
    memset( _cbFunc, 0, sizeof(_cbFunc) );

    _asptr_mgr = new CPtrMgr<CActionSet, PtrType_New>;
    _msgptr_mgr = new CPtrMgr<CMsgBase, PtrType_New>;
}

CAsyncFrame::~CAsyncFrame()
{
    if( _pNeedDeleteAS != NULL )
    {
        _asptr_mgr->Recycle( _pNeedDeleteAS );
        _pNeedDeleteAS = NULL;
    }

    delete _asptr_mgr;
    delete _msgptr_mgr;

    ClearStateMap();

    CRouteMgr::Destroy();
}

// 初始化异步框架
int CAsyncFrame::InitFrame(CServerBase *pServBase, 
        CPollerUnit *pPollerUnit,
        CTimerUnit *pTimerUnit,
        int nPendingConnNum /*= 100*/)
{
    if( NULL == pServBase
            || NULL == pPollerUnit
            || NULL == pTimerUnit )
    {
        return -1;
    }

    _pServBase = pServBase;
	CDefaultWorker* worker = (CDefaultWorker*)_pServBase;

    CRouteMgr::Instance()->Init(this, OnRouteCompleted);

    CNetMgr::Instance()->Init(pPollerUnit, pTimerUnit, this, nPendingConnNum, worker->get_TOS());

    return 0;
}

int CAsyncFrame::InitFrame2(CServerBase *pServBase, 
        int nPendingConnNum /*= 100*/,
        int nOverloadLimit /*= 0*/,
        int nL5Timeout /*= 100*/,
        unsigned short nL5Port /*= 8888*/)
{
    CPollerUnit* pPollerUnit = SPP_ASYNC::GetPollerUnit();
    CTimerUnit* pTimerUnit = SPP_ASYNC::GetTimerUnit();
    if( NULL == pPollerUnit
            || NULL == pTimerUnit )
    {
        return -1;
    }
    
    if( nOverloadLimit < 0 )
    {
        nOverloadLimit = 0;
    }
    _nOverloadLimit = nOverloadLimit;

    int ret = InitFrame(pServBase, pPollerUnit, pTimerUnit, nPendingConnNum); 
    if( ret != 0 )
    {
        return -2;
    }

    // Add log for unused warning
    FRAME_LOG(LOG_TRACE, "Init L5 timeout %d, port %u\n", nL5Timeout, nL5Port);

    return 0;

}

// 反初始化异步框架
void CAsyncFrame::FiniFrame()
{
    CNetMgr::Destroy();

    CRouteMgr::Destroy();
}

// 注册框架回调函数
int CAsyncFrame::RegCallback(CBType type, CBFunc func)
{
    int ret = 0;
    switch( type )
    {
        case CBType_Init:
            _cbFunc[CBType_Init] = func;
            break;
        case CBType_Fini:
            _cbFunc[CBType_Fini] = func;
            break;
        case CBType_Overload:
            _cbFunc[CBType_Overload] = func;
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

// 添加IState派生类对象
int CAsyncFrame::AddState( int id, IState *pState)
{
    if( id <= 0 
            || NULL == pState)
    {
        return -1;
    }

    StateMap::iterator it = _mapState.find( id );
    if( it != _mapState.end() )
    {
        return -2;
    }

    _mapState.insert( StateMap::value_type( id, pState) );

    return 0;
}

// 清除IState派生类对象
void CAsyncFrame::ClearStateMap()
{
    StateMap::iterator it = _mapState.begin();
    for(; it != _mapState.end(); it++ )
    {
        delete it->second;
    }

    _mapState.clear();
}

// 处理请求
int CAsyncFrame::Process(CMsgBase *pMsg)
{
    if( NULL == pMsg ) {
        return -1;
    }

    if( _nOverloadLimit > 0 
            && _nRequestCount >= _nOverloadLimit )
    {// 过载保护
        if (_cbFunc[CBType_Overload] != NULL )
        {   
            FRAME_LOG(LOG_ERROR, "Overload CallBack; Msg: %p; OverloadLimit: %d\n", pMsg, _nOverloadLimit);
            _cbFunc[CBType_Overload]( this, pMsg );
        }

        delete pMsg;
        return 0;
    }

    _msgptr_mgr->Register( pMsg );
    _nRequestCount++;   // 请求计数加一

    int nStateID = 1;   // 1为默认初始状态
    if(_cbFunc[CBType_Init] != NULL )
    {
        FRAME_LOG(LOG_TRACE, "Init CallBack; Msg: %p\n", pMsg);
        nStateID = _cbFunc[CBType_Init]( this, pMsg);

        if( nStateID <= 0 )
        {// nStateID非法，结束请求处理
            if (_cbFunc[CBType_Fini] != NULL )
            {   
                FRAME_LOG(LOG_TRACE, "Fini CallBack; Msg: %p\n", pMsg);
                _cbFunc[CBType_Fini]( this, pMsg );
            }

            _msgptr_mgr->Recycle( pMsg );
            _nRequestCount--;   // 请求计数减一

            return 0;

        }
    }

    // 启动状态nStateID处理
    int ret = HandleEncode( nStateID, pMsg);
    if( ret != 0)
    {
        FRAME_LOG(LOG_ERROR, "HandleEncode Fail, StateID: %d, ret: %d\n", nStateID, ret);

        if (_cbFunc[CBType_Fini] != NULL )
        {   
            FRAME_LOG(LOG_TRACE, "Fini CallBack; Msg: %p\n", pMsg);
            _cbFunc[CBType_Fini]( this, pMsg);
        }

        _msgptr_mgr->Recycle( pMsg );
        _nRequestCount--;   // 请求计数减一
    }

    return 0;
}

// 启动状态处理
int CAsyncFrame::HandleEncode(int id, CMsgBase *pMsg)
{

    StateMap::iterator it = _mapState.find(id);
    if( it == _mapState.end() )
    {// ID为id的状态没有找到
        FRAME_LOG(LOG_ERROR, "Find no state: %d, Msg: %p\n", id, pMsg);
        return -1;
    }

    IState *pState = it->second;

    FRAME_LOG(LOG_TRACE, "Switch To State: %d, IState: %p, Msg: %p\n", id, pState, pMsg);

    CActionSet *pActionSet = new CActionSet( this, pState, pMsg );

    // 启动状态处理
    int ret = pState->HandleEncode( this, pActionSet, pMsg );
    if( ret != 0 )
    {
        FRAME_LOG(LOG_ERROR, "IState::HandleEncode Fail, StateID: %d, Msg: %p\n", 
                        id, pMsg);
        delete pActionSet;
        return -2;
    }

    _asptr_mgr->Register( pActionSet ) ;

    if( pActionSet->GetActionNum() > 0 )
    {
        // 启动动作进行异步处理
        pActionSet->StartAction();
    }
    else
    {
        // 当没有Action需要处理时，同步调用IState::HandleProcess方法
        // 这种没有Action的State可以用来执行同步逻辑处理

        HandleProcess( pState, pActionSet, pMsg );
    }

    return 0;
}
int CAsyncFrame::HandleProcess(IState *pState, CActionSet *pActionSet, CMsgBase *pMsg )
{
    FRAME_LOG(LOG_TRACE, "State Process Completed. IState: %p, Msg: %p\n", pState, pMsg);

    // 回调IState::HandleProcess方法，让插件处理动作执行结果
    int nNextState = pState->HandleProcess(this, pActionSet, pMsg );

    if( _pNeedDeleteAS != NULL )
    {
        _asptr_mgr->Recycle( _pNeedDeleteAS );
        _pNeedDeleteAS = NULL;
    }

    // 不能立即删除pActionSet， 因为调用CAsyncFrame::HandleProcess的上层函数有可能还需要此对象指针
    _pNeedDeleteAS = pActionSet;

    //_asptr_mgr->Recycle( pActionSet );

    if( nNextState <= 0 )
    {// 请求处理完毕
        FRAME_LOG(LOG_TRACE, "All State Process Complete, Msg: %p\n", pMsg);
        if (_cbFunc[CBType_Fini] != NULL )
        {
            FRAME_LOG(LOG_TRACE, "Fini CallBack; Msg: %p\n", pMsg);
            _cbFunc[CBType_Fini]( this, pMsg );
        }

        _msgptr_mgr->Recycle( pMsg );
        _nRequestCount--;   // 请求计数减一
    }
    else
    {// 转换到下一个状态处理
        int ret = HandleEncode( nNextState, pMsg );
        if( ret != 0)
        {
            if (_cbFunc[CBType_Fini] != NULL )
            {   
                FRAME_LOG(LOG_TRACE, "Fini CallBack; Msg: %p\n", pMsg);
                _cbFunc[CBType_Fini]( this, pMsg );
            }

            _msgptr_mgr->Recycle( pMsg );
            _nRequestCount--;   // 请求计数减一
        }

    }

    return 0;
}


/**
 *  @brief Get msg count processing
 *  @return msg count process
 */
int CAsyncFrame::GetMsgCount()
{
    return _nRequestCount;
}


