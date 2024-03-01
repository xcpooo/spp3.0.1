#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "timerlist.h"
#include "session_mgr.h"
#include "general_session.h"
#include "session_mgr.h"
#include"../comm/singleton.h"

using namespace std;
using namespace tbase::tcommu;
using namespace ASYNC_SPP_II;
using namespace::tbase::tlog;
using namespace::spp::singleton;


GeneralSession::GeneralSession(const int sid, 
                                const char* type, 
                                const char* subtype, 
                                const char* ip, 
                                const int port,
                                const int recv_cnt, 
                                const int timeout, 
                                const int multi_con_inf, 
                                const int multi_con_sup)
{
    std::string	 main_stype(type), sub_stype(subtype);

    if (main_stype == "ttc")
        _maintype = MAIN_STYPE_TTC;
    else if (main_stype == "tdb")
        _maintype = MAIN_STYPE_TDB;
    else if (main_stype == "custom")
        _maintype = MAIN_STYPE_CUSTOM;
    else
        _maintype = MAIN_STYPE_UNKNOWN;

    if (sub_stype == "tcp")
        _subtype = SUB_STYPE_TCP;
    else if (sub_stype == "udp")
        _subtype = SUB_STYPE_UDP;
    else if (sub_stype == "tcp_multi_con")
        _subtype = SUB_STYPE_TCP_MULTI_CON;
    else
        _subtype = SUB_STYPE_UNKNOWN;

    _status	= SESSION_UN_INIT;
    _id 	= sid;
    _ip		= ip;
    _port	= port;
    _timeout = timeout;
    _recv_count = recv_cnt;
    _long_connect = (_recv_count == 1 ? false : true);

    _mainCon = NULL;

    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        if (multi_con_inf <= 0)
            _multi_con_inf = DEFAULT_MULTI_CON_INF;
        else
            _multi_con_inf = multi_con_inf;

        if (multi_con_sup <= 0)
            _multi_con_sup = DEFAULT_MULTI_CON_SUP;
        else
            _multi_con_sup = multi_con_sup;

        if (_multi_con_sup < _multi_con_inf)
            _multi_con_sup = _multi_con_inf;

        _idle_con_set = new set<int>;
        _con_map = new CON_MAP;
    } else {
        _multi_con_inf = 1;
        _multi_con_sup = 1;
        _idle_con_set = NULL;
        _con_map = NULL;
    }

    _recv_f = NULL;
    _input_f = NULL;
    _proc_f = NULL;
    _pack_cb_f = NULL;
    _recv_param = _input_param = _proc_param = _pack_cb_global_param = NULL;
    _idle_timer=NULL;
	//设置数据流session
	_session_type = SESSION_DATA;
}

//add by jeremy
GeneralSession::GeneralSession(const int sid, 
                                    int fd, 
                                    raw_handle_func input_f, 
                                    void* input_param,
                                    raw_handle_func output_f, 
                                    void* output_param, 
                                    raw_handle_func exception_f, 
                                    void* exception_param)
{
	_status = SESSION_UN_INIT;
	_id = sid;

	_fd = fd;

	_fd_input_f = input_f;
	_fd_input_param = input_param;

	_fd_output_f = output_f;
	_fd_output_param = output_param;

	_fd_exception_f = exception_f;
	_fd_exception_param = exception_param;

	//其他数据成员是否要全部为空，除了_id
	_mainCon = NULL;

	//设置控制流session
	_session_type = SESSION_CONTROL;	

}

void  GeneralSession::Attach()
{
    if (_timeout <= 0)
        return;

    AttachTimer(CSessionMgr::Instance()->get_session_timer(_timeout));
}


void GeneralSession::add_active_con(CClientUnit* con)
{
    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        _idle_con_set->insert(con->get_cid());
    }
}


void GeneralSession::del_active_con(CClientUnit* con)
{
    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {

        _idle_con_set->erase(con->get_cid());
    }
}


int GeneralSession::init_cons(int TOS)
{
    int con_index = 0;
	_TOS = TOS;
    
	if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        if (_con_map->size() != 0)
            return SPP_ASYNC::ASYNC_GENERAL_ERR;

        for (con_index = 0; con_index < _multi_con_inf; con_index++) {
            CClientUnit *cu = new CClientUnit(this, -1, con_index, TOS);

            if (cu == NULL)
                return SPP_ASYNC::ASYNC_FATAL_ERR;

            cu->do_connect();
            _con_map->insert(CON_MAP::value_type(con_index, cu));
            INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] init,add con[%d],fd:%d\n", _id, con_index, cu->get_netfd());
        }

    } else {
        if (_mainCon != NULL)
            return SPP_ASYNC::ASYNC_GENERAL_ERR;

        _mainCon = new CClientUnit(this, -1, con_index, TOS);

        if (_mainCon == NULL)
            return SPP_ASYNC::ASYNC_FATAL_ERR;

        _mainCon->do_connect();
        INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] add con[%d],fd:%d\n", _id, con_index, _mainCon->get_netfd());
    }

    return SPP_ASYNC::ASYNC_SUCC;
}

//add by jeremy
int GeneralSession::init_cons(int fd, int TOS)
{
	int con_index = 0;
    _TOS = TOS;

	if(_mainCon != NULL)
		return SPP_ASYNC::ASYNC_GENERAL_ERR;

	_mainCon = new CClientUnit(this, fd, con_index, TOS);

	if(_mainCon == NULL)
		return SPP_ASYNC::ASYNC_FATAL_ERR;

	_mainCon->set_io_state(CONN_CONNECTED);
	_mainCon->EnableOutput();
	_mainCon->Attach();
    
	INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] add con[%d],fd:%d\n", _id, con_index, _mainCon->get_netfd());

	return SPP_ASYNC::ASYNC_SUCC;
}

// not used
int GeneralSession::add_con(int con_id, int fd)
{
    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        CON_MAP::iterator con_ite = _con_map->find(con_id);

        if (con_ite != _con_map->end()) {
            close(fd);
            return -1;
        }
    } else if (_mainCon != NULL) {
        close(fd);
        return -1;
    }

    CClientUnit *cu = new CClientUnit(this, fd, con_id, -1);

    if (cu == NULL)
        return -1;

    cu->DisableInput();
    cu->DisableOutput();

    if (cu->Attach() == -1) {
        delete cu;
        return -1;
    }

    INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] add con[%d],fd:%d\n", _id, con_id, fd);

    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        _con_map->insert(CON_MAP::value_type(con_id, cu));
    } else
        _mainCon = cu;

    return 0;
}

void GeneralSession::remove_con(int con_id)
{
    if (_subtype != SUB_STYPE_TCP_MULTI_CON) {
        if (con_id != 0)
            return;

        _mainCon = NULL;
    }

    _con_map->erase(con_id);
    _idle_con_set->erase(con_id);

}

int GeneralSession::send_data(char* buf, int buf_len, void* input_param, void* proc_param, void* pack_info)
{
	int cid = 0, fd = 0;

	if(_session_type == SESSION_DATA) {

		if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
			if (!_idle_con_set->empty()) {
				set<int>::iterator con_ite = _idle_con_set->begin();
				cid = *con_ite;
				_idle_con_set->erase(con_ite);
			} else {
				if (_con_map->size() >= (unsigned)_multi_con_sup)
					return -1;
					
				cid = _con_map->rbegin()->first + 1;
				CClientUnit *cu = new CClientUnit(this, -1, cid, _TOS);

				if (cu == NULL)
					return SPP_ASYNC::ASYNC_FATAL_ERR;
	
				_con_map->insert(CON_MAP::value_type(cid, cu));

				if(cu->do_connect( MAX_CONNECT_DELAY_US)!=SPP_ASYNC::ASYNC_SUCC
				  ||cu->get_io_state()!= CONN_CONNECTED)
					return -1;

				_idle_con_set->erase(cid);
			}

			CClientUnit* con = (*_con_map)[cid];
			con->set_local_input_param(input_param);
			con->set_local_proc_param(proc_param);
			return con->AddSendBuf(buf, buf_len, pack_info);
		}


		if (_mainCon->get_io_state() != CONN_CONNECTED)
			return -1;

		return _mainCon->AddSendBuf(buf, buf_len, pack_info);
	}
	else if(_session_type == SESSION_CONTROL){
		INTERNAL_LOG->LOG_P(LOG_DEBUG, "control session[%d] in send data, connect id[%d],fd:%d\n", _id, cid, fd);
		return -1;
	}
	return -1;
}

void GeneralSession::recycle_con(int cid)
{
	if(_session_type == SESSION_DATA) {
		if (_subtype == SUB_STYPE_TCP_MULTI_CON)
			_idle_con_set->insert(cid);
	}
	else if(_session_type == SESSION_CONTROL) {
		INTERNAL_LOG->LOG_P(LOG_DEBUG, "control session[%d] in recycle con, connect id[%d]", _id, cid);
	}
}

GeneralSession::~GeneralSession()
{
    if (_subtype == SUB_STYPE_TCP_MULTI_CON) {
        _idle_con_set->clear();

        for (CON_MAP::iterator con_ite = _con_map->begin(); con_ite != _con_map->end(); ++con_ite) {
            //close(cit->second->get_netfd());
            CClientUnit* con = con_ite->second;

            if (con == CSessionMgr::Instance()->get_in_process_con()) {
                con_ite->second->set_con_state(CON_STATE_TERMINATE);
                continue;
            }

            delete con_ite->second;
        }

        delete _idle_con_set;
        delete _con_map;
    } else {
        if (_mainCon != NULL) {
            if (_mainCon != CSessionMgr::Instance()->get_in_process_con())
                delete _mainCon;
            else
	    {
		_mainCon->DetachPoller();
                _mainCon->set_con_state(CON_STATE_TERMINATE);
	    }
        }
    }

    DisableTimer();

    INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] destroy\n", _id);
}

void GeneralSession::TimerNotify(void)
{
    CSessionMgr::Instance()->destroy_session(_id);
}
