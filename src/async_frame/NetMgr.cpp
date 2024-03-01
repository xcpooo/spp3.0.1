/**
 * @file NetMgr.cpp
 * @brief 网络管理模块
 * @author aoxu, aoxu@tencent.com
 * @version 1.1
 * @date 2011-10-09
 */
#include "NetMgr.h"
#include "NetHandler.h"
#include "AsyncFrame.h"
#include "ParallelNetHandler.h"
#include <coresingleton.h>

#define C_DEFAULT_MAX_NUM 3
#define CHECK_IDLE_HANDLER_INTERVAL 300  //5MIN

USING_ASYNCFRAME_NS;

bool handler_idx::operator<(const handler_idx& right)const
{
    int ret = strcmp(_ip, right._ip) ;
    if( ret < 0 ) {
        return true;
    } else if( ret > 0 ) {
        return false;
    }

    if( _port < right._port ) {
        return true;
    } else if ( _port > right._port  ) {
        return false;
    }

    return _proto < right._proto;
}

handler_array::handler_array(const unsigned max_num) : _cur_num(0), _max_num(max_num), _aryHandler(NULL)
{
    _aryHandler = (CNetHandler **)malloc( _max_num*sizeof(CNetHandler *) );
    memset( _aryHandler, 0, _max_num*sizeof(CNetHandler *) );
}

handler_array::~handler_array()
{
    for( unsigned i = 0; i < _cur_num; i++)
    {
        if (_aryHandler[i] != NULL)
        {
            delete _aryHandler[i];
            _aryHandler[i] = NULL;
        }
    }

    free( _aryHandler );
}

parallel_handler_array::parallel_handler_array(const unsigned max_num) :
    _cur_num(0), _max_num(max_num), _aryHandler(NULL)
{
    if(max_num < 1)
    {
        _max_num = C_DEFAULT_MAX_NUM;
    }

    _aryHandler = (CParallelNetHandler **)malloc( _max_num*sizeof(CParallelNetHandler *) );
    memset( _aryHandler, 0, _max_num*sizeof(CParallelNetHandler *) );
}

parallel_handler_array::~parallel_handler_array()
{
    for( unsigned i = 0; i < _cur_num; i++)
    {
        if (_aryHandler[i] != NULL)
        {
            delete _aryHandler[i];
            _aryHandler[i] = NULL;
        }
    }

    free( _aryHandler );
}

CNetMgr * CNetMgr::_instance = NULL;

CNetMgr* CNetMgr::Instance (void)
{
    if( NULL == _instance )
    {
        _instance = new CNetMgr;
    }

    return _instance;
}

void CNetMgr::Destroy(void)
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

int CNetMgr::Init(CPollerUnit *pollerUnit,
                        CTimerUnit *timerUnit,
                        CAsyncFrame *pFrame,
                        int PendingConnNum,
						int TOS)
{
    _pollerUnit = pollerUnit;
    _timerUnit = timerUnit;
    _pFrame = pFrame;
    _MaxPendingConnNum = PendingConnNum;
    if(_MaxPendingConnNum < 1 ) {
        _MaxPendingConnNum = 10;
    }
	_TOS = TOS;
    _timerList = timerUnit->GetTimerList(CHECK_IDLE_HANDLER_INTERVAL * 1000);
    if (_timerList != NULL)
    {
        AttachTimer(_timerList);
    }
    return 0;
}

void CNetMgr::Fini()
{
    {
        HandlerMap::iterator it = _mapHandler.begin();
        for(; it != _mapHandler.end(); it++ )
        {
            std::set<CNetHandler*>::iterator it2 = it->second.begin();
            for( ; it2 != it->second.end(); it2++ )
            {
                delete (*it2);
            }

            it->second.clear();
        }
        _mapHandler.clear();
    }

    {
        PendingHandlerMap::iterator it = _mapPendingHandler.begin();
        for(; it != _mapPendingHandler.end(); it++ )
        {
            delete it->second;
        }
        _mapPendingHandler.clear();
    }

    {
        ParallelHandlerMap::iterator it = _mapParallelHandler.begin();
        for (;it != _mapParallelHandler.end(); it ++)
        {
            delete it->second;
        }
        _mapParallelHandler.clear();
    }

    if ( _nhptr_mgr != NULL )
    {
        delete _nhptr_mgr;
    }
}

CNetMgr::CNetMgr()
        : _pollerUnit(NULL), _timerUnit(NULL), _pFrame(NULL)
        , _nhptr_mgr(NULL), _MaxPendingConnNum(10), _TOS(-1)
{
    _nhptr_mgr = new CPtrMgr<CNetHandler, PtrType_New>();
}

CNetMgr::~CNetMgr()
{
    Fini();
}

CNetHandler *CNetMgr::GetHandler(const std::string &ip,
        PortType port,
        ProtoType proto,
        ConnType conn_type)
{
    handler_idx_t idx;
    strncpy( idx._ip, ip.c_str(), sizeof(idx._ip)-1 );
    idx._port = port;
    idx._proto = proto;

    CNetHandler *pHandler = NULL;

    if( conn_type == ConnType_Short
            || conn_type == ConnType_Long )
    {
        HandlerMap::iterator it = _mapHandler.find(idx);
        if( it == _mapHandler.end() )
        {
            pHandler = new CNetHandler(_pollerUnit, _timerUnit,
                    ip, port, proto, conn_type, _TOS);
        }
        else
        {
            if(  it->second.size() > 0 )
            {
                std::set<CNetHandler*>::iterator it2 = it->second.begin();
                pHandler = *it2;
                it->second.erase( it2 );
            }
            else
            {
                pHandler = new CNetHandler(_pollerUnit, _timerUnit,
                        ip, port, proto, conn_type, _TOS);
            }
        }

        _nhptr_mgr->Register(pHandler);
    }
    else if(conn_type == ConnType_LongWithPending)
    {
        PendingHandlerMap::iterator it = _mapPendingHandler.find(idx);
        if( it == _mapPendingHandler.end() )
        {
            pHandler = new CNetHandler(_pollerUnit, _timerUnit,
                    ip, port, proto, conn_type, _TOS);
            handler_array_t* aryHandler = new handler_array_t((unsigned)_MaxPendingConnNum);
            aryHandler->_aryHandler[aryHandler->_cur_num++] = pHandler;

            _mapPendingHandler.insert( PendingHandlerMap::value_type(idx, aryHandler) );
        }
        else
        {
            handler_array_t* aryHandler = it->second;
            int choice =  (int)(aryHandler->_max_num * (rand() / (RAND_MAX + 1.0)));
            if (aryHandler->_aryHandler[choice] == NULL)
            {
                pHandler = new CNetHandler(_pollerUnit, _timerUnit,
                        ip, port, proto, conn_type, _TOS);
                aryHandler->_aryHandler[choice] = pHandler;
                aryHandler->_cur_num++;
            }
            else
            {
                //已经有handler了
                pHandler = aryHandler->_aryHandler[choice];
            }
        }
    }

    return pHandler;
}

CParallelNetHandler *CNetMgr::GetParallelHandler(const std::string &ip,
        PortType port,
        ProtoType proto,
        ConnType conn_type)
{
    handler_idx_t idx;
    strncpy( idx._ip, ip.c_str(), sizeof(idx._ip)-1 );
    idx._port = port;
    idx._proto = proto;

    CParallelNetHandler *pHandler = NULL;

    ParallelHandlerMap::iterator it = _mapParallelHandler.find(idx);
    if( it == _mapParallelHandler.end() )
    {
        pHandler = new CParallelNetHandler(_pollerUnit, _timerUnit,
                ip, port, proto, conn_type, _TOS);

        parallel_handler_array_t* aryHandler = new parallel_handler_array_t(GetConnNumLimit(idx));
        aryHandler->_aryHandler[aryHandler->_cur_num++] = pHandler;

        _mapParallelHandler.insert( ParallelHandlerMap::value_type(idx, aryHandler) );
    }
    else
    {
        parallel_handler_array_t* aryHandler = it->second;
        if( aryHandler->_cur_num < aryHandler->_max_num)
        {
            pHandler = new CParallelNetHandler(_pollerUnit, _timerUnit,
                    ip, port, proto, conn_type, _TOS);

            aryHandler->_aryHandler[aryHandler->_cur_num++] = pHandler;
        }
        else
        {
            int choice = (int) ( aryHandler->_cur_num * (rand() / (RAND_MAX + 1.0)));
            pHandler = aryHandler->_aryHandler[choice];
        }
    }

    return pHandler;
}

void CNetMgr::RecycleHandler(CNetHandler *pHandler)
{
    std::string ip;
    PortType port;
    ProtoType proto;
    ConnType conn_type;

    pHandler->GetDestIP(ip);
    pHandler->GetDestPort(port);
    pHandler->GetProtoType(proto);
    pHandler->GetConnType(conn_type);

    handler_idx_t idx;
    strncpy(idx._ip, ip.c_str(), sizeof(idx._ip)-1 );
    idx._port = port;
    idx._proto = proto;

    if( conn_type == ConnType_Short
            || conn_type == ConnType_Long )
    {
        _nhptr_mgr->Recycle(pHandler, false);

        HandlerMap::iterator it = _mapHandler.find(idx);
        if( it == _mapHandler.end() )
        {
            std::pair<HandlerMap::iterator, bool> ret = _mapHandler.insert(
                    HandlerMap::value_type(idx, std::set<CNetHandler*>() ) );

            ret.first->second.insert(pHandler);
        }
        else
        {
            it->second.insert( pHandler );
        }
    }
}

void CNetMgr::InitConnNumLimit(const std::string&ip, const unsigned short port, const unsigned int limit)
{
    handler_idx_t idx;
    strncpy( idx._ip, ip.c_str(), sizeof(idx._ip) - 1 );
    idx._port = port;
    idx._proto = ProtoType_TCP; // 仅TCP需要限制连接上限

    InitConnNumLimit(idx, limit);
}

unsigned CNetMgr::GetConnNumLimit(const std::string&ip, const unsigned short port)
{
    handler_idx_t idx;
    strncpy( idx._ip, ip.c_str(), sizeof(idx._ip) - 1 );
    idx._port = port;
    idx._proto = ProtoType_TCP; // 仅TCP需要限制连接上限

    return GetConnNumLimit(idx);
}

void CNetMgr::InitConnNumLimit(const handler_idx_t& idx, const unsigned int limit)
{
    ParallelHandlerMap::iterator it = _mapParallelHandler.find(idx);
    if( it == _mapParallelHandler.end() )
    {
        // 不存在连接池，需要创建
        parallel_handler_array_t* aryHandler = new parallel_handler_array_t(limit);

        _mapParallelHandler.insert( ParallelHandlerMap::value_type(idx, aryHandler) );
    } // else 存在，什么也不做
}

unsigned CNetMgr::GetConnNumLimit(const handler_idx_t& idx)
{
    ParallelHandlerMap::iterator it = _mapParallelHandler.find(idx);
    if( it == _mapParallelHandler.end() )
    {
        // 不存在连接池，返回0
        return 0;
    }
    else
    {
        parallel_handler_array_t* aryHandler = it->second;
        return aryHandler->_max_num;
    }
}

void CNetMgr::TimerNotify(void)
{
    CheckIdleHander();
    if (_timerList != NULL)
    {
        AttachTimer(_timerList);
    }
    return;
}
void CNetMgr::CheckIdleHander(void)
{
    {
        HandlerMap::iterator it = _mapHandler.begin();
        for (; it != _mapHandler.end(); )
        {
            std::set<CNetHandler*>::iterator it2 = it->second.begin();
            for (;it2 != it->second.end(); )
            {
                CNetHandler* handler = *it2;
                //如果该handler可回收, 就把他从set中擦掉
                if (handler->CanRecycle())
                {
                    handler->Recycle();
                    delete handler;
                    it->second.erase(it2 ++);
                }
                else
                {
                    it2 ++;
                }
            }
            //如果该set为空, 则把<handler_idx_t, std::set<CNetHandler*>
            //从map中回收
            if (it->second.size() == 0)
            {
                _pFrame->FRAME_LOG(LOG_TRACE, "Recycle Handler Set(%s, %u, %s)\n",
                        it->first._ip, it->first._port, ProtoTypeToStr(it->first._proto));
                _mapHandler.erase(it ++);
            }
            else
            {
                it ++;
            }
        }

    }
    {
        PendingHandlerMap::iterator it = _mapPendingHandler.begin();
        for (; it != _mapPendingHandler.end(); )
        {
            handler_array_t* aryHandler = it->second;
            for (unsigned i = 0; i < aryHandler->_max_num; i ++)
            {
                if (aryHandler->_aryHandler[i] != NULL)
                {
                    if (aryHandler->_aryHandler[i]->CanRecycle())
                    {
                        aryHandler->_aryHandler[i]->Recycle();
                        delete aryHandler->_aryHandler[i];
                        aryHandler->_aryHandler[i] = NULL;
                        aryHandler->_cur_num -- ;
                    }
                }
            }
            if (aryHandler->_cur_num == 0)
            {
                _pFrame->FRAME_LOG(LOG_TRACE, "Recycle Pending Handler Set(%s, %u, %s)\n",
                        it->first._ip, it->first._port, ProtoTypeToStr(it->first._proto));

                delete it->second;
                _mapPendingHandler.erase(it++);
            }
            else
            {
                it ++;
            }
        }
    }
    return;
}
