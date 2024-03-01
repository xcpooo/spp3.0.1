/************************************************************
    FileName   ParallelNetHandler.cpp
    Author     kylexiao
    Version    1.0
    Create     2011/10/08
    DESC:      多发多收NetHandler
    History
        <author>    <time>      <version >  <desc>

    Copyright:  Tencent, China
*************************************************************/

#include "ParallelNetHandler.h"
#include "ActionInfo.h"
#include "AsyncFrame.h"
#include "NetMgr.h"
#include "RouteMgr.h"
#include "../comm/exception.h"
#include "../comm/monitor.h"
#include "../comm/global.h"
#include <misc.h>
#include <fcntl.h>
#include <assert.h>

USING_ASYNCFRAME_NS;
USING_L5MODULE_NS;
using namespace spp::exception;
using namespace spp::comm;
using namespace spp::global;

extern int g_spp_L5us;                // NetHandler.cpp, 2.10.8 release 2

#define PNH_LOG CNetMgr::Instance()->_pFrame->GetServerBase()->log_
#define PNH_LOG_FUNC PNH_LOG.LOG_P_ALL
#define PNH_TRACE_LOG(fmt, args...) \
    do{ if( _log_trace_on ) PNH_LOG_FUNC(LOG_TRACE, fmt, ##args); } while(0)

#define POOL_FREE_THRESHOLD (32)

#define NOW_TIME_MS __spp_get_now_ms()

#define ADD_TO_SENDING_LIST(_SE_) \
        ++_sending_cnt;\
        _sending_list.push_back(_SE_)

#define DELETE_FROM_SENDING_LIST(_IT_SE_) \
        --_sending_cnt;\
        _sending_elem_pool.push_back(*_IT_SE_);\
        _sending_list.erase(_IT_SE_)

#define DELETE_FISRT_OF_SENDING_LIST() \
        --_sending_cnt;\
        _sending_elem_pool.push_back( _sending_list.front() );\
        _sending_list.pop_front()

#define ADD_TO_RECVING_LIST(_AC_) \
        ++_recving_cnt;\
        _recving_list.push_back(_AC_)

#define DELETE_FROM_RECVING_LIST(_IT_AC_) \
        --_recving_cnt;\
        (*_IT_AC_)->DisableTimer();\
        _recving_elem_pool.push_back(*_IT_AC_);\
        _recving_list.erase(_IT_AC_)

#define RECYCLE_AC(_AC_) \
        _AC_->DisableTimer();\
        _recving_elem_pool.push_back(_AC_)

BEGIN_ASYNCFRAME_NS
// SEXInterface.cpp
extern uint64_t GetCurrentSeq();
extern void SetCurrentSeq(uint64_t seq);
END_ASYNCFRAME_NS

CParallelNetHandler::CParallelNetHandler(CPollerUnit *pollerUnit,
        CTimerUnit *timerUnit,
        const std::string &ip,
        PortType port, 
        ProtoType proto,
        ConnType conn_type,
        int TOS)
    : CPollerObject(pollerUnit), _dest_ip(ip),_dest_port(port)
    , _proto(proto), _conn_type(conn_type)
    , _sending_cnt(0), _recving_cnt(0)
    , _timerUnit(timerUnit), _conn_state(UNCONNECTED), _timeout_cnt(0)
    , _r_buf(NULL), _r_buf_size(0), _r_data_len(0), _log_trace_on(false)
{
    _dest_addr.sin_family = AF_INET;
    inet_aton(_dest_ip.c_str(), &(_dest_addr.sin_addr));
    _dest_addr.sin_port = htons(_dest_port);
    _TOS = TOS;

    _r_buf_size = 1 << 16; // 64k, max size of udp
    _r_buf = (char*)malloc( _r_buf_size );

    _s_pending_size = 1 << 13; // 8k
    _s_pending_buf = (char*)malloc( _s_pending_size );

    int log_level = PNH_LOG.log_level(-1);
    if ( log_level == LOG_TRACE )
    {
        _log_trace_on = true;
    }
}

CParallelNetHandler::~CParallelNetHandler()
{
    Close();

    if ( _r_buf )
    {
        free( _r_buf );
    }
    if ( _s_pending_buf )
    {
        free( _s_pending_buf );
    }
    _r_buf = NULL;
    _s_pending_buf = NULL;

#define DELETE_STL_ALL(CTN) \
    while ( !CTN.empty() )\
    { delete CTN.back(); CTN.pop_back(); }

    DELETE_STL_ALL(_sending_list);
    DELETE_STL_ALL(_sending_elem_pool);
    DELETE_STL_ALL(_recving_list);
    DELETE_STL_ALL(_recving_elem_pool);

}

int CParallelNetHandler::HandleRequest(CActionInfo *ai)
{
    static uint64_t last_seq = 0;
    uint64_t seq = GetCurrentSeq(); // 用户在HandleEncode时已设置
    if ( last_seq == seq )
    {
        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u]last_seq[%llu] == this_seq[%llu] ?\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, last_seq, seq);
    }
    last_seq = seq;

    PNH_TRACE_LOG("[%p][%s:%s:%u] Add Action[%p] seq[%llu]\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, ai, seq);

    CActionContext* ac;
    if ( _recving_elem_pool.empty() )
    {
        ac = new CActionContext(this);
    }
    else
    {// pool
        ac = _recving_elem_pool.back();
        _recving_elem_pool.pop_back();
        if (_recving_elem_pool.size() > POOL_FREE_THRESHOLD)
        { // LAZY FREE
            delete _recving_elem_pool.back();
            _recving_elem_pool.pop_back();
        }
    }

    ac->pAI = ai;
    ac->seq = seq;
    ac->start_time = NOW_TIME_MS;        
    int timeout = 0;
    ai->GetTimeout( timeout );        
    CTimerList *tl = _timerUnit->GetTimerList(timeout);
    if(tl != NULL)
    {
        ac->AttachTimer( tl );// Attach Action to Timer (Not NetHandler)
    }

    send_elem_t* se;
    if ( _sending_elem_pool.empty() )
    {
        se = new send_elem_t;
    }
    else
    {// pool
        se = _sending_elem_pool.back();
        _sending_elem_pool.pop_back();
        if (_sending_elem_pool.size() > POOL_FREE_THRESHOLD)
        { // LAZY FREE
            delete _sending_elem_pool.back();
            _sending_elem_pool.pop_back();
        }   
    }
    se->ac = ac;
    se->seq = seq;
    se->sent = 0;
    ai->GetBuffer( &se->buf, se->len );

    int ret = 0;
    if( UNCONNECTED == _conn_state)
    {
        ret =  Open();
    }
    else if ( _sending_list.empty() )
    {
        EnableOutput();
        ApplyEvents();
    }
    
    ADD_TO_SENDING_LIST( se );

    if ( CONNECTED == _conn_state )
    {
        ret = DoSend();
    }

    PNH_TRACE_LOG("[%p][%s:%s:%u][Status] sending#[%d] recving#[%d]\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port,
                    _sending_cnt, _recving_cnt);

    return ret;
}

int CParallelNetHandler::Open()
{
    PNH_TRACE_LOG("[%p][%s:%s:%u] create socket\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);

    // 创建socket fd
    if( netfd <= 0 ) {
        if( _proto == ProtoType_TCP ) {
            netfd = socket(AF_INET, SOCK_STREAM, 0);
        } else {
            netfd = socket(AF_INET, SOCK_DGRAM, 0);
        }

        if(netfd <= 0) {
            MONITOR(MONITOR_WORKER_SOCKET_FAIL); //create socket failed
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] socket error: %m\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
            DoErrorFinished( ECreateSock );
            return -1;
        }
    }

    if(_TOS >= 0)
	    setsockopt(netfd, IPPROTO_IP, IP_TOS, &_TOS, sizeof(int));

    // 将socket fd设置为非阻塞
    SetNonBlocking(netfd);

    // 打开Output/Input事件
    EnableOutput();
    EnableInput();

    // 绑定到PollerUnit 
    int ret = -1;
    ret = AttachPoller();
    if(-1 == ret) {
        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] AttachPoller error\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
        DoErrorFinished( EAttachPoller );
        return -1;
    }

    if ( _proto == ProtoType_TCP )
    { // tcp 
        // 开始连接后端server, 通常在OutputNotify事件第一次触发时，表明连接成功
        if( 0 != DoConnect() ) {
            DoErrorFinished( EConnect );
            return -1;
        }

    }
    else
    { // udp
        _conn_state = CONNECTED;
    }

    return 0;
}

int CParallelNetHandler::Close()
{
    PNH_TRACE_LOG("[%p][%s:%s:%u] Close the Connection!\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);

    _r_data_len = 0;
    _timeout_cnt = 0;

    DisableInput();
    DisableOutput();

    _conn_state = UNCONNECTED;

    int ret = DetachPoller();
    if(-1 == ret)
    {
    }

    if(netfd > 0)
    {
        ::close(netfd);
        netfd = -1;
    }

    return 0;
}

int CParallelNetHandler::SetNonBlocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)   {
        return -1;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)   {
        return -1;
    }

    return 0;
}


int CParallelNetHandler::DoConnect()
{
    int ret = connect(netfd, (sockaddr *)&_dest_addr, sizeof(_dest_addr));
    if(ret == -1)
    {
        switch(errno)
        {
            case EINTR :
            case EINPROGRESS :
                _conn_state = CONNECTING;
                break;
            default:
                PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] connect failed, errno: %m\n", 
                            this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
                MONITOR(MONITOR_WOKER_CONNECT_FAIL);//socket connect failed
                return -1;
        }
    }
    else
    {
        PNH_TRACE_LOG( "[%p][%s:%s:%u] connect succ\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
        _conn_state = CONNECTED;
    }
    return 0;
}


int CParallelNetHandler::InputNotify()
{
    if( _recving_list.empty() )
    {
        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] ERROR: recving list is empty\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);

        DoErrorFinished( EInvalidState );
        return -1;
    }

    return DoRecv();
}

int CParallelNetHandler::OutputNotify()
{
    if( _conn_state != CONNECTED )
    {
        if( _conn_state == CONNECTING )
        {
	    	int error = 0;
	    	socklen_t len = sizeof(error);
			if (::getsockopt(netfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error!=0) 
			{
		        PNH_LOG_FUNC( LOG_ERROR,  "[%p][%s:%s:%u] connect failed (%m)\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);		
		        DoErrorFinished( EConnect);
		        return -1;			
			}
        
            PNH_TRACE_LOG( "[%p][%s:%s:%u] connect succ\n", 
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
            _conn_state = CONNECTED;
            
        }
        else
        {
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] ERROR _conn_state[%d]\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, _conn_state);
            DoErrorFinished( EInvalidState );
            return -2;
        }
    }

    if( _sending_list.empty() )
    {// 上周期已处理完
        DisableOutput();
        ApplyEvents();
        return 0;
    }

    return DoSend();

}

int CParallelNetHandler::HangupNotify()
{
    PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] Hangup!\n", 
                        this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
    DoErrorFinished( EHangup );
    return 0;
}

void CParallelNetHandler::TimerNotify(uint64_t seq)
{
    _timeout_cnt++;

    CActionContext* ac;
    std::list<CActionContext*>::iterator    it_ac;
    std::list<send_elem_t*>::iterator       it_se;

    for (it_ac = _recving_list.begin(); it_ac != _recving_list.end(); ++it_ac)
    {// find from recving list
        if ( (*it_ac)->seq == seq )
        {
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] recving seq[%llu] Timeout\n",
                            this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, seq);
            ac = *it_ac;
            DELETE_FROM_RECVING_LIST( it_ac );
            goto TAG_NOTIFY_ACTION;
        }
    }

    if ( !_sending_list.empty() )
    {// find from sending list
        it_se = _sending_list.begin();
        if ( (*it_se)->seq == seq )
        {
            send_elem_t* se = *it_se;
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] sending seq[%llu] Timeout sent[%d/%d]\n",
                this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, seq, se->sent, se->len);

            ac = se->ac;
            if ( se->sent != 0 )
            {// 发送到一半时超时，把剩下的发送再回收。
                CopyToPending( se );
            }
            else
            {
                DELETE_FROM_SENDING_LIST( it_se );
            }
            RECYCLE_AC( ac );
            goto TAG_NOTIFY_ACTION;
        }

        for (++it_se; it_se != _sending_list.end(); ++it_se)
        {
            if ( (*it_se)->seq == seq )
            {
                PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] sending seq[%llu] Timeout sent[%d/%d]\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, seq, (*it_se)->sent, (*it_se)->len);

                ac = (*it_se)->ac;
                DELETE_FROM_SENDING_LIST( it_se );
                RECYCLE_AC( ac );
                goto TAG_NOTIFY_ACTION;
            }
        }
    }

    PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] seq[%llu] not found!\n", 
                        this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, seq);
    return;

TAG_NOTIFY_ACTION:
    NotifyAction(seq, ETimeout, ac->pAI, NOW_TIME_MS - ac->start_time);
    if ( _recving_list.empty() && _sending_list.empty() ) 
    {
        this->Close();
    }

    // if timetout over limit, close link
    if ((spp_global::link_timeout_limit != 0) && (_timeout_cnt >= spp_global::link_timeout_limit))
    {
        PNH_LOG_FUNC(LOG_FATAL, "[%p][%s:%s:%u] timeout[%d] over limit[%d]!\n", 
             this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, _timeout_cnt, spp_global::link_timeout_limit);
        DoErrorFinished(EOverLinkErrLimit); 
    }
    
    return;
}

void CParallelNetHandler::CopyToPending(send_elem_t* se)
{
// 当正在发送的Action，发生错误时（如超时）
// ActionInfo的buf可能会失效，这里要用_s_pending_buf代替

    int need_resize = 0;
    while ( se->len - se->sent > _s_pending_size )
    {
        need_resize = 1;
        _s_pending_size <<= 1; // double it
    }

    if ( need_resize )
    {
		_s_pending_buf = (char*)CMisc::realloc_safe(_s_pending_buf, _s_pending_size);// 兼容data == NULL
    }

    int left_len = se->len - se->sent;
    memcpy( _s_pending_buf, se->buf + se->sent, left_len);
    se->buf = _s_pending_buf; 
    se->len = left_len;
    se->sent = 0;
    se->ac = NULL;// 标识该Action已经删除

// 此时，se仍在sending_list上，也就是说，仍然会发送完剩下的数据
// 但是，后端Server在回包时，由于ActionInfo已经Timeout，会导致seq找不到。

}

int CParallelNetHandler::DoSend()
{
    int ret, buf_len, sent;
    char* buf;
    send_elem_t* se;

DO_SEND_START:
    se = _sending_list.front();
    buf      = se->buf;
    buf_len  = se->len;
    sent     = se->sent;
    if ( _proto == ProtoType_TCP )
    {
        ret = ::send (netfd, buf + sent, buf_len - sent, 0);
    }
    else
    {
        ret = ::sendto(netfd, buf + sent, buf_len - sent, 0, 
                        (struct sockaddr *)&_dest_addr, sizeof(struct sockaddr_in));
    }
    
    if( ret < 0 )
    {
        if(errno==EINTR || errno==EAGAIN)
        {
            return 0;
        }

        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] send error[%m]\n", 
                        this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);

        MONITOR(MONITOR_WORKER_SEND_FAIL);//send failed
        DoErrorFinished( ESend );

        return -1;
    }
    //_timeout_cnt = 0; // reset it

    sent += ret;
    PNH_TRACE_LOG("[%p][%s:%s:%u] seq[%llu] send[%d/%d]\n", this, ProtoTypeToStr(_proto),
                        _dest_ip.c_str(), _dest_port, se->seq, sent, buf_len);

    if ( sent >= buf_len )
    {// send finished
        PNH_TRACE_LOG("seq[%llu] send finished.\n", se->seq);
        if ( se->ac )
        {
            ADD_TO_RECVING_LIST( se->ac );
        }

        DELETE_FISRT_OF_SENDING_LIST();
        if ( _sending_list.empty() )
        {
            return 0;
        }
    }
    else
    {
        se->sent = sent;// update it
    }

goto DO_SEND_START;

    return 0;
}

int CParallelNetHandler::DoRecv()
{
    int ret;

    if ( _proto == ProtoType_TCP )
    {
        ret = recv( netfd, _r_buf + _r_data_len, _r_buf_size - _r_data_len, 0 ); 
    }
    else
    {
        ret = recvfrom( netfd, _r_buf + _r_data_len, _r_buf_size - _r_data_len, 0, NULL, 0);
    }

    if( ret <= 0 )
    {
        if( ret == 0 )
        {
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] peer closed, recv 0\n", 
                                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
            DoErrorFinished( EShutdown );
            return -1;
        }

        if(errno==EINTR || errno==EAGAIN)
        {
            return 0;
        }

        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] recv error : %m\n", 
                                this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
        DoErrorFinished( ERecv );
        return -1;
    }
    //_timeout_cnt = 0; // reset it

    _r_data_len += ret;
    PNH_TRACE_LOG("[%p][%s:%s:%u] do recv[%d] buffer free[%d/%d]\n", this, ProtoTypeToStr(_proto),
                    _dest_ip.c_str(), _dest_port, ret, _r_data_len, _r_buf_size);

    return DoProcess();
}

int CParallelNetHandler::DoProcess()
{
    int processed = 0;
    while ( processed < _r_data_len && !_recving_list.empty() )
    {
        CActionInfo* ai = _recving_list.front()->pAI;
        int proto_len = ai->HandleInput( _r_buf + processed, _r_data_len - processed );
        if ( proto_len == 0 )
        {// continue to recv
            break;
        }

        if( proto_len > 0 && proto_len <= _r_data_len - processed )
        {// recv OK，正常情况下Seq在Input已设置
		    if (_spp_g_exceptionpacketdump)
            {
               PacketData stPacketData(_r_buf + processed, _r_data_len - processed, inet_addr(_dest_ip.c_str()), 0, _dest_port, 0, __spp_get_now_ms());
               SavePackage(PACK_TYPE_BACK_RECV, &stPacketData);
            }

            _timeout_cnt = 0; // reset it, only check pkg ok
            DoRecvFinished(_r_buf + processed, proto_len, GetCurrentSeq());
            processed += proto_len;
        }
        else
        {// pkg_len invaild
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] ERROR: ECheckPkg [%d][%d/%d]\n",
                                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, proto_len, processed, _r_data_len);
            DoErrorFinished( ECheckPkg );
            return -1;
        }
    }

    PNH_TRACE_LOG("processed[%d/%d]\n", processed, _r_data_len);

    if ( processed > 0 )
    {
        if ( processed >= _r_data_len || _recving_list.empty() )
        {
            _r_data_len = 0;
        }
        else
        {
            _r_data_len -= processed;
            memmove(_r_buf, _r_buf + processed, _r_data_len);
        }
    }

    if ( _r_buf_size - _r_data_len < (1<<12) )
    {// usable buffer < 4k
        _r_buf_size = _r_buf_size << 1;// double it
	char* new_r_buf = (char*)CMisc::realloc_safe(_r_buf, _r_buf_size);// 兼容data == NULL
        if (_r_buf_size > (1<<23))
        { // 8MB
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] : WARNING , recv buffer size [%d]\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, _r_buf_size);
        }

        if (new_r_buf != NULL)
        {
            _r_buf = new_r_buf;
        }
        else
        {// 内存不足
            PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] : Error , Realloc Failed.\n",
                                        this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port);
            free(_r_buf);
            _r_data_len = 0;
            _r_buf_size = 1 << 16;
            _r_buf = (char*)malloc( _r_buf_size );
            assert(_r_buf != NULL);
        }


    }

    return 0;
}

void CParallelNetHandler::DoErrorFinished(int err)
{

    PNH_TRACE_LOG("[%p][%s:%s:%u] Oops... err[ %s ]\n", this, ProtoTypeToStr(_proto),
                    _dest_ip.c_str(), _dest_port, spperrToStr(err));
    this->Close();

    uint64_t now = NOW_TIME_MS;

    CActionContext* ac;
    CActionInfo *   ai;
    std::list<CActionContext*>::iterator    it_ac;
    std::list<send_elem_t*>::iterator       it_se;

    for (it_ac = _recving_list.begin(); it_ac != _recving_list.end(); ++it_ac)
    {// notify all action in _recving_list
        ac = *it_ac;  
        ai = ac->pAI;
        PNH_LOG_FUNC( LOG_ERROR, "NotifyAction[%llu] on recving list\n", ac->seq);
        NotifyAction(ac->seq, err, ai, now - ac->start_time);
        RECYCLE_AC( ac );
    }
    _recving_cnt = 0;
    _recving_list.clear();

    if ( !_sending_list.empty() )
    {// notify a action on sending list
        ac = _sending_list.front()->ac;
        if ( ac )
        {
            ai = ac->pAI;
            PNH_LOG_FUNC( LOG_ERROR, "NotifyAction[%llu] on sending list\n", ac->seq);
            NotifyAction(ac->seq, err, ai, now - ac->start_time);
            RECYCLE_AC( ac );
        }
        DELETE_FISRT_OF_SENDING_LIST();
        if ( !_sending_list.empty() )
        {
            this->Open();
        }
    }
}


void CParallelNetHandler::DoRecvFinished(const char* buf, int len, uint64_t seq)
{
    static uint64_t last_seq = 0;
    if ( last_seq == seq )
    {
        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u]last_seq[%llu] == this_seq[%llu] ?\n",
                    this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, last_seq, seq);
    }
    last_seq = seq;

    PNH_TRACE_LOG("recv OK, seq[%llu] len[%d]\n", seq, len);

    // 多数情况下，先发的请求，先回包，因此下面能快速找出seq
    std::list<CActionContext*>::iterator it;
    for (it = _recving_list.begin();
         it != _recving_list.end() && (*it)->seq != seq ;
         ++it)
            ;

    if ( it == _recving_list.end() )
    {
        PNH_LOG_FUNC( LOG_ERROR, "[%p][%s:%s:%u] Seq[%llu] not found!\n", 
                            this, ProtoTypeToStr(_proto), _dest_ip.c_str(), _dest_port, seq);
        return;
    }

    CActionContext* ac = *it;    
    CActionInfo *ai = ac->pAI;
    NotifyAction(seq, 0, ai, NOW_TIME_MS - ac->start_time);
    ai->HandleProcess(buf, len);

    DELETE_FROM_RECVING_LIST( it );
}

void CParallelNetHandler::NotifyAction(uint64_t seq, int result, CActionInfo *ai, int timecost)
{
    SetCurrentSeq( seq ); // 设置Seq，方便用户调用
    ai->SetTimeCost( timecost );
    UpdateRouteResultIfNeed( result, ai );
    if ( result != 0 )
    {
        ai->HandleError( result );
    }
}

void CParallelNetHandler::UpdateRouteResultIfNeed(int ecode, CActionInfo * action)
{

    RouteType route_type = action->GetRouteType();
    if ( route_type == NoGetRoute )
    {
        return;
    }

    if( ecode != 0          && ecode != EConnect && 
        ecode != EHangup    && ecode != EShutdown &&
        ecode != ECheckPkg  && ecode != ETimeout && 
        ecode != ESend      && ecode != ERecv) 
    {
        ecode = 0;
    }

    int modid = 0, cmdid = 0, cost = 0;
    std::string host_ip;
    unsigned short host_port = 0;

    action->GetDestIp( host_ip );
    action->GetDestPort( host_port );
    action->GetTimeCost(cost);
    if( cost < 1 ) {
        cost = 1;
    }

    // add 2.10.8 release2, for L5 api report
    if (g_spp_L5us != 0) {
        cost *= 1000;    
    }

    switch( route_type )
    {
        case L5NonStateRoute:
            {
                CNonStateActionInfo *pActionInfo = (CNonStateActionInfo *)action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        case L5StateRoute:
            {
                CStateActionInfo *pActionInfo = (CStateActionInfo *)action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        case L5AntiParalRoute:
            {
                CAntiParalActionInfo *pActionInfo = (CAntiParalActionInfo*)action;
                pActionInfo->GetRouteID(modid, cmdid);
                CRouteMgr::Instance()->UpdateRouteResult(modid, cmdid, host_ip, host_port, ecode, cost);
                break;
            }
        default:
            break;
    }

}
