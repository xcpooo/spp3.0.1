#include "sessionmgrconf.h"
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "timerlist.h"
#include "general_session.h"
#include "session_mgr.h"
#include "asyn_api.h"
#include "notify.h"

using namespace std;
using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace ASYNC_SPP_II;
using namespace spp::singleton;
using namespace tbase::tlog;
using namespace spp::tconfbase;

extern struct timeval __spp_g_now_tv;

int CSessionMgr::init_mgr(void)
{
    _pollerunit = new CPollerUnit(TGlobal::_maxpoller);

    if (NULL == _pollerunit) {
        return -1;
    }

    _timerunit = new CTimerUnit();

    _reconnect_timerunit = new CTimerUnit();

    if (NULL == _timerunit) 
    {
        return -1;
    }

    if (NULL == _reconnect_timerunit)
    {
        return -1;
    }

    set_reconnect_timer();

    if (_pollerunit->InitializePollerUnit() < 0) {
        return -1;
    }

    return 0;
}

using namespace tbase::notify;
class CNotifyClient
    : public CNotify
    , public CPollerObject
{
public:
    CNotifyClient():
        CPollerObject(NULL)
    {
    }

    int AttachKey(int key)
    {
        if ( netfd > 0 )
        {
            return netfd;
        }

        int ret = CNotify::notify_init(key);
        if ( ret > 0 )
        {
            netfd = ret;
            EnableInput();
            DisableOutput();
            return 0;
        }
        return ret;
    }

    virtual int InputNotify()
    {
        return notify_recv(netfd);
    }
};

static CNotifyClient notifier;

int CSessionMgr::init_notify(int key)
{
    int ret = notifier.AttachKey(key);
    if (ret == 0)
    {
        return notifier.AttachPoller(_pollerunit);
    }
    return ret;
}

void CSessionMgr::set_reconnect_timer(int interval)
{
    if (interval <= 0)
        return ;

    if (_reconnect_timerlist == NULL || interval != TGlobal::_reconnect_interval) {
        TGlobal::_reconnect_interval = interval;
        CTimerList* new_timer = _reconnect_timerunit->GetTimerList(TGlobal::_reconnect_interval);

        if (_reconnect_timerlist != NULL) {
            _reconnect_timerlist->TransforTimer(new_timer);
            delete _reconnect_timerlist;
        }
        _reconnect_timerlist = new_timer;
    }
}


int CSessionMgr::set_session_idle_timer(int timeout,int sid)
{
	CTimerList* timer=NULL;
	GS_ITE gs_ite;
	if(timeout<=0)
	{
  		return SPP_ASYNC::ASYNC_SUCC; 
	}

	if(sid==-1)
	{
		_globalidletimer=_timerunit->GetTimerList(timeout);
		for(gs_ite=_SessionMap.begin();gs_ite!=_SessionMap.end();gs_ite++)
		{
			gs_ite->second->_idle_timer=_globalidletimer;
		}
    		return  SPP_ASYNC::ASYNC_SUCC;
	}

	gs_ite = _SessionMap.find(sid);
    	if (gs_ite == _SessionMap.end()) 
	{
        	return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    	}
	
	timer=_timerunit->GetTimerList(timeout);
	gs_ite->second->_idle_timer=timer;
    	return  SPP_ASYNC::ASYNC_SUCC;
}


int CSessionMgr::close(void)
{
    delete _timerunit;
    delete _pollerunit;
    return 0;
}


inline int CSessionMgr::disable_notify(void)
{
    notifier.DisableInput();
    return notifier.ApplyEvents();
}

inline int CSessionMgr::enable_notify(void)
{
    notifier.EnableInput();
    return notifier.ApplyEvents();
}

int CSessionMgr::run(bool isBlock)
{
    __spp_do_update_tv(); // 第一次更新时间
    uint64_t now = __spp_get_now_ms();

    if ( !_isEnableAsync ) // 同步
    {
        if ( isBlock )
        {
            _pollerunit->WaitPollerEvents( _max_epoll_timeout );
            _pollerunit->ProcessPollerEvents();
        }
    }
	else	//异步
	{
	    if ( isBlock )
	    {
	        if ( _epwaittimeout >= 3 )
	        {// 找出将在DEFAULT_EPOLL_WAIT_TIME内超时的请求
	            enable_notify();
	            _epwaittimeout = _timerunit->ExpireMicroSeconds( DEFAULT_EPOLL_WAIT_TIME ) + 1;// 多睡1ms防止临界
	            if (_epwaittimeout > _max_epoll_timeout)
	            {
	                _epwaittimeout = _max_epoll_timeout;
	            }
	        }
	        else
	        {
	            ++_epwaittimeout;
	        }

	        _pollerunit->WaitPollerEvents( _epwaittimeout );

	        __spp_do_update_tv(); // (epoll_wait可能经过了N个ms）
	    }
	    else
	    {
	        disable_notify();
	        _epwaittimeout = 0;
	        _pollerunit->WaitPollerEvents( _epwaittimeout );
	    }

	    _pollerunit->ProcessPollerEvents();
	    _reconnect_timerunit->CheckExpired(now);
	}

	if (_isEnableAsync || _isEnableTimer)
	    _timerunit->CheckExpired(now);

    return 0;
}

// the return value means: how many connections are added in the pool
int CSessionMgr::create_session(const int sid, const char* type, const char* subtype, const char* ip, const int port, const int recv_cnt, const int timeout, const int multi_con_inf, const int multi_con_sup)
{
    _isEnableAsync = true;

    int err_ret = SPP_ASYNC::ASYNC_REG_SESSION_ERR ;
    int reg_sid = -1;
    GeneralSession* gs = new GeneralSession(sid, type, subtype, ip, port, recv_cnt, timeout, multi_con_inf, multi_con_sup);

    if (gs == NULL)
        goto SESSION_REG_FAIL;

    //check maintype and subtype
    if (gs->check() == false) {
        goto SESSION_REG_FAIL;
    }

    if (gs->_id > 0) {
        if (check_reserve_sid(gs->_id) == false) {
            err_ret = SPP_ASYNC:: ASYNC_REG_ID_OVERLAP;
            goto SESSION_REG_FAIL;
        }

        reg_sid = gs->_id;

    } else {
        reg_sid = alloc_sid();

        if (reg_sid == ERR_SESSION_ID)
            goto SESSION_REG_FAIL;
    }

    gs->_id = reg_sid;

    _SessionMap.insert(GS_MAP::value_type(gs->_id, gs));
    //_UnConnectedSessionMap.insert(GS_MAP::value_type(gs->_id, gs));

    err_ret = gs->init_cons(_TOS);

    if (err_ret != SPP_ASYNC::ASYNC_SUCC) {
        _SessionMap.erase(gs->_id);
        err_ret = SPP_ASYNC::ASYNC_CONNECT_ERR;
        goto SESSION_REG_FAIL;
    }

    gs->Attach();
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] create succcessful\n", reg_sid);
    return reg_sid;

SESSION_REG_FAIL:
    delete gs;
    return err_ret;
}

//add by jeremy
int CSessionMgr::create_session(const int sid, int fd, raw_handle_func input_f, void* input_param,
		                        raw_handle_func output_f, void* output_param,
								raw_handle_func exception_f, void* exception_param)
{
    _isEnableAsync = true;

	int err_ret = SPP_ASYNC::ASYNC_REG_SESSION_ERR;
	int reg_sid = -1;
	GeneralSession* gs = new GeneralSession(sid, fd, input_f, input_param, output_f, output_param, exception_f, exception_param);

	if(gs == NULL)
		goto SESSION_REG_FAIL;

	if(gs->_id > 0) {
		if(check_reserve_sid(gs->_id) == false) {
			err_ret = SPP_ASYNC::ASYNC_REG_ID_OVERLAP;
			goto SESSION_REG_FAIL;
		}

		reg_sid = gs->_id;
	}
	else {

		reg_sid = alloc_sid();

		if(reg_sid == ERR_SESSION_ID) {
			goto SESSION_REG_FAIL;
		}
	}

	gs->_id = reg_sid;

	_SessionMap.insert(GS_MAP::value_type(gs->_id, gs));
	
	//确认最后释放
	//_UnConnectedSessionMap.insert(GS_MAP::value_type(gs->_id, gs));

	err_ret = gs->init_cons(fd, _TOS);

	if(err_ret != SPP_ASYNC::ASYNC_SUCC) {
		_SessionMap.erase(gs->_id);
		err_ret = SPP_ASYNC::ASYNC_CONNECT_ERR;
		goto SESSION_REG_FAIL;
	}

	INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] create successful\n", reg_sid);
	return reg_sid;

SESSION_REG_FAIL:
	delete gs;
	return err_ret;
}

int CSessionMgr::create_tm_session(const int sid, const int cb_interval, void* cb_param, time_task_func cb_func)
{
    _isEnableTimer = true;

    int err_ret = SPP_ASYNC::ASYNC_REG_SESSION_ERR;
    int reg_sid = -1;
    TmSession* tms = NULL;
    CTimerList* timerlist = NULL;

    if (cb_func == NULL)
        return err_ret;

	if(cb_interval <= 0)
		return err_ret;

    tms = new TmSession(sid, cb_interval, cb_param, cb_func);

    if (tms == NULL)
        return err_ret;

    if (tms->_id > 0) {
        if (check_reserve_tm_sid(tms->_id) == false) {
            err_ret = SPP_ASYNC:: ASYNC_REG_ID_OVERLAP;
            goto TM_SESSION_REG_FAIL;
        }

        reg_sid = tms->_id;

    } else {
        reg_sid = alloc_tm_sid();

        if (reg_sid == ERR_SESSION_ID)
            goto TM_SESSION_REG_FAIL;
    }

    tms->_id = reg_sid;
    timerlist = _timerunit->GetTimerList(cb_interval);

    if (timerlist == NULL)
        goto TM_SESSION_REG_FAIL;

    tms->AttachTimer(timerlist);
    _TmSessionMap.insert(TMS_MAP::value_type(tms->_id, tms));
    return reg_sid;

TM_SESSION_REG_FAIL:
    delete tms;
    return err_ret;
}




int CSessionMgr::send_data(int sid, char* buf, int len, void* input_param, void* proc_param, void* pack_info)
{
    GS_ITE gs_ite = _SessionMap.find(sid);

    if (gs_ite == _SessionMap.end()) {
        return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    }

    if (gs_ite->second->_status == SESSION_UN_INIT) {
        return	SPP_ASYNC::ASYNC_SEND_FAILED;
    }

    int ret = gs_ite->second->send_data(buf, len, input_param, proc_param, pack_info);

    return ret == 0 ? SPP_ASYNC::ASYNC_SUCC : SPP_ASYNC::ASYNC_SEND_FAILED;
}

int CSessionMgr::reg_session_callBack(int sid, process_func proc_f, void* proc_param, input_func input_f, void* input_param)
{
    if (proc_f == NULL || input_f == NULL) {
        return SPP_ASYNC::ASYNC_PARAM_ERR;
    }

    GS_ITE gs_ite = _SessionMap.find(sid);

    if (gs_ite == _SessionMap.end()) {
        return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    }

    GeneralSession* s = gs_ite->second;
    recv_func recv_f = NULL;

    switch (s->_maintype) {
        case GeneralSession::MAIN_STYPE_TTC:
            recv_f = ttc_recv;
            break;
        case GeneralSession::MAIN_STYPE_TDB:
            recv_f = tdb_recv;
            break;
        case GeneralSession::MAIN_STYPE_CUSTOM:

            switch (s->_subtype) {
                case GeneralSession::SUB_STYPE_TCP:
                case GeneralSession::SUB_STYPE_TCP_MULTI_CON:
                    recv_f = tcp_recv;
                    break;
                case GeneralSession::SUB_STYPE_UDP:
                    recv_f = udp_recv;
                    break;
                default:
                    break;
            }

            break;
        default:
            break;

    }

    if (recv_f == NULL) {
        return SPP_ASYNC::ASYNC_PARAM_ERR;
    }

    s->reg_callback(recv_f, NULL, proc_f, proc_param, input_f, input_param);
    s->_status = SESSION_IDLE;

    return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int CSessionMgr::reg_session_callBack(const int sid, 
                                        raw_handle_func input_f, 
                                        void* input_param, 
                                        raw_handle_func output_f, 
                                        void* output_param, 
                                        raw_handle_func exception_f, 
                                        void* exception_param)
{
	if(input_f == NULL || output_f == NULL || exception_f == NULL) {
		return SPP_ASYNC::ASYNC_PARAM_ERR;
	}

	GS_ITE gs_ite = _SessionMap.find(sid);

	if(gs_ite == _SessionMap.end()) {
		return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
	}

	GeneralSession* s = gs_ite->second;

	s->reg_callback(input_f, input_param, output_f, output_param, exception_f, exception_param);
	s->_status = SESSION_IDLE;

	return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int CSessionMgr::EnableInput(const int sid)
{
	GS_ITE gs_ite = _SessionMap.find(sid);

	if(gs_ite == _SessionMap.end()) {
		return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
	}

	GeneralSession* s = gs_ite->second;

	s->get_client_unit()->EnableInput();
	s->get_client_unit()->ApplyEvents();

	return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int CSessionMgr::DisableInput(const int sid)
{
	GS_ITE gs_ite = _SessionMap.find(sid);

	if(gs_ite == _SessionMap.end()) {
		return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
	}

	GeneralSession* s = gs_ite->second;

	s->get_client_unit()->DisableInput();
	s->get_client_unit()->ApplyEvents();

	return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int CSessionMgr::EnableOutput(const int sid)
{
	GS_ITE gs_ite = _SessionMap.find(sid);

	if(gs_ite == _SessionMap.end()) {
		return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
	}

	GeneralSession* s = gs_ite->second;

	s->get_client_unit()->EnableOutput();
	s->get_client_unit()->ApplyEvents();

	return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int CSessionMgr::DisableOutput(const int sid)
{
	GS_ITE gs_ite = _SessionMap.find(sid);

	if(gs_ite == _SessionMap.end()) {
		return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
	}

	GeneralSession* s = gs_ite->second;

	s->get_client_unit()->DisableOutput();
	s->get_client_unit()->ApplyEvents();

	return SPP_ASYNC::ASYNC_SUCC;
}

int CSessionMgr::reg_pack_callBack(int sid, pack_cb_func proc, void* global_param)
{
    if (proc == NULL) {
        return SPP_ASYNC::ASYNC_PARAM_ERR;
    }

    GS_ITE gs_ite = _SessionMap.find(sid);

    if (gs_ite == _SessionMap.end()) {
        return SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    }

    GeneralSession* s = gs_ite->second;
    s->reg_pack_callback(proc, global_param);

    return SPP_ASYNC::ASYNC_SUCC;
}

// the return value means: how much connections are removed from the pool
int CSessionMgr::destroy_session(const int sid)
{
    GS_MAP::iterator gs_ite = _SessionMap.find(sid);

    if (gs_ite == _SessionMap.end()) {
        return  SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    }

    GeneralSession* s = gs_ite->second;
    delete s;
    _SessionMap.erase(gs_ite);

    return SPP_ASYNC::ASYNC_SUCC;
}

int CSessionMgr::destroy_tm_session(const int sid)
{
    TMS_ITE tms_ite = _TmSessionMap.find(sid);

    if (tms_ite == _TmSessionMap.end()) {
        return  SPP_ASYNC::ASYNC_SESSION_NONE_EXIST;
    }

    TmSession* s = tms_ite->second;
    s->DisableTimer();

    if (s->GetStatus() == SESSION_INPROCESS) {
        s->SetStatus(SESSION_TERMINATED);
    } else {
        delete s;
    }

    _TmSessionMap.erase(tms_ite);

    return SPP_ASYNC::ASYNC_SUCC;
}

int CSessionMgr::init(string config_file, int max_epoll_timeout, int TOS)
{
    _max_epoll_timeout = max_epoll_timeout;
    if( _max_epoll_timeout < MIN_EPOLL_WAIT_TIME )
    {
        _max_epoll_timeout = MIN_EPOLL_WAIT_TIME;
    }

    _TOS = TOS;

    if( config_file.empty() )
    {
        return SPP_ASYNC::ASYNC_SUCC;
    }

    CSessionMgrConf conf(new CLoadXML(config_file));

    if (conf.init() != 0) {

        return SPP_ASYNC::ASYNC_GENERAL_ERR;
    }

    AsyncSession session;   
    int ret = conf.getAndCheckAsyncSession(session);
    if (ret != 0) {
        return SPP_ASYNC::ASYNC_GENERAL_ERR;
    }

    for (size_t i = 0; i < session.entry.size(); ++ i)
    {
        create_session(session.entry[i].id, session.entry[i].maintype.c_str(), session.entry[i].subtype.c_str(), 
                session.entry[i].ip.c_str(), session.entry[i].port,
                session.entry[i].recv_count, session.entry[i].timeout, session.entry[i].multi_con_inf, session.entry[i].multi_con_sup);
    }
    return SPP_ASYNC::ASYNC_SUCC;
}


CSessionMgr* CSessionMgr::Instance()
{
    return SingleTon<CSessionMgr>::Instance();
}

int CSessionMgr::recycle_con(int sid , int cid)
{
    if (sid == -1 && cid == -1) {
        if (_in_process_con == NULL || _in_process_con->get_con_state() == CON_STATE_TERMINATE ||
            _in_process_con->get_owner()->_subtype != GeneralSession::SUB_STYPE_TCP_MULTI_CON ||
            _in_process_con->get_event() != EVENT_NORMAL) {

            INTERNAL_LOG->LOG_P(LOG_DEBUG, " recycle s[%d] c[%d] failed \n", sid, cid);
            return SPP_ASYNC::ASYNC_GENERAL_ERR;

        }

        _in_process_con->recycle();

    } else {
        return SPP_ASYNC::ASYNC_GENERAL_ERR;
    }

    return SPP_ASYNC::ASYNC_SUCC;
}

bool CSessionMgr::check_reserve_sid(int id)
{
    if (id < RESERVED_SESSION_ID_INF || id > RESERVED_SESSION_ID_SUP) {
        return false;
    } else {
        GS_ITE s_ite;

        if ((s_ite = _SessionMap.find(id)) != _SessionMap.end()) {
            return false;
        }

        return true;
    }
}

bool CSessionMgr::check_reserve_tm_sid(int id)
{
    if (id < RESERVED_SESSION_ID_INF || id > RESERVED_SESSION_ID_SUP) {
        return false;
    } else {
        TMS_ITE s_ite;

        if ((s_ite = _TmSessionMap.find(id)) != _TmSessionMap.end()) {
            return false;
        }

        return true;
    }
}

int  CSessionMgr::alloc_sid()
{
    int chosen_id = _free_s_id;
    GS_ITE s_ite;

    while ((s_ite = _SessionMap.find(chosen_id)) != _SessionMap.end()) {
        chosen_id = (chosen_id == MAX_SESSION_ID ? MIN_SESSION_ID : chosen_id + 1);

        if (chosen_id == _free_s_id)
            return ERR_SESSION_ID;
    }

    _free_s_id = (chosen_id == MAX_SESSION_ID ? MIN_SESSION_ID : chosen_id + 1);
    return chosen_id;
}

int  CSessionMgr::alloc_tm_sid()
{
    int chosen_id = _free_tms_id;
    TMS_ITE s_ite;

    while ((s_ite = _TmSessionMap.find(chosen_id)) != _TmSessionMap.end()) {
        chosen_id = (chosen_id == MAX_SESSION_ID ? MIN_SESSION_ID : chosen_id + 1);

        if (chosen_id == _free_tms_id)
            return ERR_SESSION_ID;
    }

    _free_tms_id = (chosen_id == MAX_SESSION_ID ? MIN_SESSION_ID : chosen_id + 1);
    return chosen_id;
}


void CSessionMgr::set_user_data(int flow, void* data)
{
    if (data == NULL)
        _user_data_map.erase(flow);
    else
        _user_data_map[flow] = data;
}

void* CSessionMgr::get_user_data(int flow)
{
    USER_DATA_MAP::const_iterator it = _user_data_map.find(flow);

    if (it == _user_data_map.end())
        return NULL;
    else
        return it->second;
}

// Run timerunit, check expired 
void CSessionMgr::check_expired()
{
    __spp_do_update_tv();
    uint64_t now = __spp_get_now_ms();
    _timerunit->CheckExpired(now);
}


