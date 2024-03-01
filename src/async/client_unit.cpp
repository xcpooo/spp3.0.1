#include <stdio.h>
#include <sys/un.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include"timerlist.h"
#include"poller.h"
#include <client_unit.h>
#include"../comm/tbase/tcommu.h"
#include"../comm/singleton.h"
#include "general_session.h"
#include "session_mgr.h"


#define MAX_ERR_RSP_MSG_LEN 4096

using namespace tbase::tcommu::tsockcommu;
using namespace tbase::tcommu;
using namespace tbase::tlog;
using namespace spp::singleton;
using namespace ASYNC_SPP_II;

static const int  C_READ_BUFFER_SIZE = 64 * 1024;
static char RecvBuf[C_READ_BUFFER_SIZE];

CClientUnit::CClientUnit(GeneralSession* s, int fd, int con_id, int TOS):
        CPollerObject(CSessionMgr::Instance()->get_pollerunit(), fd),
        _io_state(CONN_IDLE),
        _sid(s->_id),
        _cid(con_id),
        _have_sent(0),
        _total_send(0),
        _owner(s),
        _event(EVENT_NORMAL),
        _local_input_param(NULL),
        _local_proc_param(NULL)
{
    _cache = new CRawCache(CSessionMgr::Instance()->get_mempool());
    memset(&_com_info, 0, sizeof(_com_info));
    _com_info.remoteport_ = s->_port;
    in_addr ip_addr;
    inet_aton(s->_ip.c_str(), &ip_addr);
    _com_info.remoteip_ = ip_addr.s_addr;
	_TOS = TOS;
}

CClientUnit::~CClientUnit(void)
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] destroyed ,send_buf size: %d\n", _sid, _cid, _send_que.size());
    DetachPoller();
    DisableTimer();
    clear_send_buf();
    delete _cache;
}


int CClientUnit::do_connect(int timeout)
{
	if (netfd > 0) {
		DetachPoller();
		close(netfd);
	}

    _cache->skip( _cache->data_len() );

	if (_owner->_subtype == GeneralSession::SUB_STYPE_TCP || _owner->_subtype == GeneralSession::SUB_STYPE_TCP_MULTI_CON)
		netfd = socket(PF_INET, SOCK_STREAM, 0);
	else if (_owner->_subtype == GeneralSession::SUB_STYPE_UDP)
		netfd = socket(PF_INET, SOCK_DGRAM, 0);
	else
		return SPP_ASYNC::ASYNC_CONNECT_ERR;
    
	if(_TOS >= 0) // 0 <= TOS <= 255
        setsockopt(netfd, IPPROTO_IP, IP_TOS, &_TOS, sizeof(int));

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, _owner->_ip.c_str(), &addr.sin_addr);
	addr.sin_port = htons(_owner->_port);
	struct timeval tv;
	
	tv.tv_sec = 0;
	if(timeout<=0)
	{	
		tv.tv_usec = MAX_NET_TRANSFER_DELAY_US;
		int opts = fcntl(netfd, F_GETFL);
		if (opts >= 0) {
			opts |= O_NONBLOCK;
			fcntl(netfd, F_SETFL, opts);
		}	
		setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	}
	else
	{
		tv.tv_sec = 0;

		tv.tv_usec = timeout;

		setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	}

	_io_state = CONN_CONNECTING;

	if(_owner->_subtype == GeneralSession::SUB_STYPE_TCP|| _owner->_subtype == GeneralSession::SUB_STYPE_TCP_MULTI_CON)
	{	
		int con_ret=connect(netfd, (struct sockaddr*)&addr, sizeof(addr));
			 
		tv.tv_usec = MAX_NET_TRANSFER_DELAY_US;
		int opts = fcntl(netfd, F_GETFL);
		if (opts >= 0) {
		opts |= O_NONBLOCK;
		fcntl(netfd, F_SETFL, opts);
		}	
		setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
			
		if (con_ret < 0) 
		{
			if (errno != EINTR && errno != EINPROGRESS) 
			{
				INTERNAL_LOG->LOG_P(LOG_DEBUG, "session[%d] connect[%s:%d] error %m\n",
						_owner->_id, _owner->_ip.c_str(), _owner->_port);
				close(netfd);
				netfd = -1;
				_io_state = CONN_DISCONNECT;
				update_reconnect_timer();	
				return SPP_ASYNC::ASYNC_SUCC;
			}

			update_reconnect_timer();
			EnableOutput();
			goto ATTACH_CON;
		}
	}
	
	_io_state = CONN_CONNECTED;
	_owner->add_active_con(this);
	update_idle_timer();

ATTACH_CON:
	EnableInput();
	Attach();
	return SPP_ASYNC::ASYNC_SUCC;
}


int CClientUnit::Attach(void)
{
    EnableInput();		//是否多余,见上方

    if (AttachPoller() == -1) {
        INTERNAL_LOG->LOG_P(LOG_ERROR, "Fatal Error:cu[%d:%d] attach failed \n", _sid, _cid);
        return -1;
    }

    return 0;
}


int CClientUnit::handle_recv(int& recv_len)
{
    int ret = CON_SUCC;
    int total_recv_len = 0;
    int recv_ret = 0;

    if (_owner->_recv_f == NULL) {
        recv_len = 0;
        return CON_SUCC;
    }

    do {

        recv_ret = _owner->_recv_f(_event, _owner->_id, &netfd, RecvBuf, C_READ_BUFFER_SIZE);

        if (_logic_state == CON_STATE_TERMINATE)
            return  CON_QUIT;

        if (_logic_state == CON_STATE_IDLE)
            return CON_BACK_TO_IDLE;

        if (recv_ret > 0) {
            total_recv_len += recv_ret;
            _cache->append(RecvBuf, recv_ret);

            if (recv_ret == C_READ_BUFFER_SIZE)
                continue;

            ret = CON_SUCC;
            break;
        } else if (recv_ret < 0) {
            INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] disconnect,should be shutdown\n", _sid, _cid);
            ret = CON_ERR_SHUTDOWN;
            break;
        } else if (recv_ret == 0) {
            ret = CON_SUCC;
            break;
        }
    } while (1);

    recv_len = total_recv_len;
    return ret;
}


int CClientUnit::handle_input(int& msg_len)
{
    int total_data_len = _cache->data_len();

    if (total_data_len == 0) {
        msg_len = 0;
        return 0;
    }

    blob_type blob;
    blob.owner = NULL;
    blob.extdata = &_com_info;
    blob.data = _cache->data();
    blob.len = total_data_len;
    void* server_base = CSessionMgr::Instance()->get_serverbase();

    void* input_param = _local_input_param == NULL ? _owner->_input_param : _local_input_param;

    if (_owner->_input_f == NULL) {
        msg_len = 0;
        return CON_SUCC;
    }

    msg_len = _owner->_input_f(input_param, _owner->_id, &blob, server_base);

    if (_logic_state == CON_STATE_TERMINATE)
        return  CON_QUIT;

    if (_logic_state == CON_STATE_IDLE)
        return CON_BACK_TO_IDLE;

    if (msg_len == 0 || (msg_len > 0 && msg_len <= total_data_len)) {
        if (msg_len > 0)
            _recv_cnt++;

        return CON_SUCC;
    }

    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] input_error,cache_data_len:%d,input ret:%d\n", _sid, _cid, total_data_len, msg_len);
    _cache->skip(total_data_len);
    return CON_INPUT_ERR;
}

int CClientUnit::handle_proc(int proc_len)
{
    void* server_base = CSessionMgr::Instance()->get_serverbase();
    blob_type blob;
    blob.owner = NULL;
    blob.extdata = &_com_info;
    blob.data = _cache->data();

    blob.len = proc_len;
    int proc_ret = 0;

    void* proc_param = _local_proc_param == NULL ? _owner->_proc_param : _local_proc_param;

    if (_owner->_proc_f != NULL)
        proc_ret = _owner->_proc_f(_event, _owner->_id, proc_param, &blob, server_base);

    if (_logic_state == CON_STATE_TERMINATE)
        return  CON_QUIT;

    if (_logic_state == CON_STATE_IDLE)
        return CON_BACK_TO_IDLE;

    if (proc_ret == 0) {
        _cache->skip(proc_len);
        return CON_SUCC;
    }

    return CON_PROC_ERR;
}

int CClientUnit::handle_pack_cb(SESSION_EVENT e, PACK_BLOB& pack)
{
    void* server_base = CSessionMgr::Instance()->get_serverbase();
    blob_type blob;
    blob.owner = NULL;
    blob.extdata = &_com_info;
    blob.data = (char*)pack._data;
    blob.len = pack._data_len;
    int ret = _owner->_pack_cb_f(e, _owner->_id, _owner->_pack_cb_global_param, pack._cb_info, &blob, server_base);
    return ret;
}


int CClientUnit::handle_event(SESSION_EVENT e)
{
	int ret = 0;
	set_event(e);

	switch (e) {
		case  EVENT_SEND_SUCC:
		case  EVENT_SEND_FAIL:

			if (_owner->_pack_cb_f != NULL) {
				PACK_BLOB& pack = _send_que.front();
				ret = handle_pack_cb(e, pack);

				if (pack._data != NULL)
					free(pack._data);

				_send_que.pop();
			} else {
				PACK_BLOB& pack = _send_que.front();
				if(pack._data != NULL)
					free(pack._data);
				_send_que.pop();
			}

			break;
		case  EVENT_SHUTDOWN_COM:
		case  EVENT_TIMEOUT:
		case  EVENT_OVERACCESS:
		case  EVENT_INPUT_ERR:

			for (int i = _send_que.size(); i > 0; i--)
			{
				PACK_BLOB& pack = _send_que.front();
				if(_owner->_pack_cb_f!=NULL)
					handle_pack_cb(EVENT_SEND_FAIL, pack);

				if (pack._data != NULL)
					free(pack._data);

				_send_que.pop();
			}

			_owner->del_active_con(this);
			ret = handle_proc(_cache->data_len());
			break;
		default:
			ret = handle_proc(_cache->data_len());
	}

	return ret;
}


int CClientUnit::InputNotify(void)
{
	INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] InputNotify \n", _sid, _cid);
	if (_io_state == CONN_CONNECTING)
	{
		_io_state = CONN_CONNECTED;
		_owner->add_active_con(this);
		DisableTimer();
		DisableOutput();
		return 0;
	}

	set_con_state(CON_STATE_IN_PROCESS);
	CSessionMgr::Instance()->set_in_process_con(this);
	update_idle_timer();

	int ret = CON_SUCC;

	if (_owner->get_session_type() == SESSION_CONTROL)
	{
		void* server_base = CSessionMgr::Instance()->get_serverbase();

		void* param = _owner->_fd_input_param;

		if (_owner->_fd_input_f == NULL)
			return CON_SUCC;

		int fd=netfd;
		int ret = _owner->_fd_input_f(_owner->_id, &fd, param, server_base);

		if (_logic_state == CON_STATE_TERMINATE)
			goto INPUT_DESTROY_CON;

		if(fd == -1||ret<0)
		{
			netfd=fd;
			INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] destroy \n", _sid, _cid);
			CSessionMgr::Instance()->set_in_process_con(NULL);
			CSessionMgr::Instance()->destroy_session(_owner->_id);
			return -1;
		}

		CSessionMgr::Instance()->set_in_process_con(NULL);
		return ret;
	}
	else if (_owner->get_session_type() == SESSION_DATA)
	{
		int recv_len = 0;
		int input_len = 0;
		ret = handle_recv(recv_len);

		switch (ret) 
		{
			case	CON_QUIT:
				goto INPUT_DESTROY_CON;
			case 	CON_ERR_SHUTDOWN:
				ret = handle_event(EVENT_SHUTDOWN_COM);

				if (ret == CON_QUIT)
					goto INPUT_DESTROY_CON;
				else
					goto INPUT_RECONNECT_CON;

			case	CON_BACK_TO_IDLE:
				goto INPUT_BACK_TO_IDLE;
			case	CON_SUCC:
				break;
		}

		do 
		{
			set_event(EVENT_NORMAL);
			ret = handle_input(input_len);

			switch (ret)
			{
				case 	CON_QUIT:

					INTERNAL_LOG->LOG_P(LOG_DEBUG, "session_destroy invoked in cu [%d:%d] when input \n", _sid, _cid);
					goto INPUT_DESTROY_CON;

				case	CON_BACK_TO_IDLE:
					INTERNAL_LOG->LOG_P(LOG_DEBUG, "recyecle_con  invoked in cu [%d:%d] when input \n", _sid, _cid);
					goto INPUT_BACK_TO_IDLE;

				case 	CON_INPUT_ERR:
					ret = handle_event(EVENT_INPUT_ERR);

					if (ret == CON_QUIT)
						goto INPUT_DESTROY_CON;
					else
						goto INPUT_RECONNECT_CON;

				case	CON_SUCC:

					if (input_len == 0)
					{
						set_event(EVENT_NORMAL);
						set_con_state(CON_STATE_BUSY);
						CSessionMgr::Instance()->set_in_process_con(NULL);
						return 0;
					} 
					else
						break;
			}

			set_event(EVENT_NORMAL);
			ret = handle_proc(input_len);

			switch (ret)
			{
				case 	CON_QUIT:
					INTERNAL_LOG->LOG_P(LOG_DEBUG, "session_destroy invoked in cu[%d:%d] when proc \n", _sid, _cid);
					goto INPUT_DESTROY_CON;

				case	CON_BACK_TO_IDLE:
					INTERNAL_LOG->LOG_P(LOG_DEBUG, "recycle_con invoked in cu[%d:%d] when proc \n", _sid, _cid);
					goto INPUT_BACK_TO_IDLE;

				case	CON_PROC_ERR:
					goto INPUT_RECONNECT_CON;

				case	CON_SUCC:

					if (is_overaccess())
					{
						INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] overaccess\n", _sid, _cid);
						ret = handle_event(EVENT_OVERACCESS);

						if (ret == CON_QUIT)
							goto INPUT_DESTROY_CON;
						else
							goto INPUT_RECONNECT_CON;
					}

					break;
			}
		} while (1);
	}

INPUT_DESTROY_CON:
	INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] destroy \n", _sid, _cid);
	CSessionMgr::Instance()->set_in_process_con(NULL);
	delete	this;
	return -1;

INPUT_RECONNECT_CON:
	INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] reconnect \n", _sid, _cid);
	ret = do_connect();

	if (ret != SPP_ASYNC::ASYNC_SUCC)
		goto INPUT_DESTROY_CON;
	else
	{
		CSessionMgr::Instance()->set_in_process_con(NULL);
		return 0;
	}

INPUT_BACK_TO_IDLE:
	CSessionMgr::Instance()->set_in_process_con(NULL);
	return 0;
}

int CClientUnit::AddSendBuf(char* buf, int len, void* pack_info)
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] add send buf\n", _sid, _cid);
    char* send_buf = (char*)malloc(len);
    memcpy(send_buf, buf, len);

    if (send_buf != NULL) {
        PACK_BLOB blob(send_buf, len, pack_info);
        _send_que.push(blob);
    } else {
        return -1;
    }

    if (_send_que.size() == 1)
   {
        EnableOutput();
        EnableInput();
        ApplyEvents();
    }

    return 0;
}


int CClientUnit::OutputNotify(void)
{
	INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] OutputNotify \n", _sid, _cid);

	if (_io_state == CONN_CONNECTING)
	{
		INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] connected remote peer\n", _sid, _cid);
		_io_state = CONN_CONNECTED;
		_owner->add_active_con(this);
		DisableOutput();
		update_idle_timer();
		return 0;
	}

	update_idle_timer();
	set_con_state(CON_STATE_IN_PROCESS);
	CSessionMgr::Instance()->set_in_process_con(this);

	if (_owner->get_session_type() == SESSION_CONTROL)
	{
		void* server_base = CSessionMgr::Instance()->get_serverbase();

		void* param = _owner->_fd_output_param;

		if (_owner->_fd_output_f == NULL)
			return CON_SUCC;

		int fd=netfd;
		int ret = _owner->_fd_output_f(_owner->_id, &fd, param, server_base);

		if (_logic_state == CON_STATE_TERMINATE)
			goto OUTPUT_DESTROY_CON;

		if(ret<0||fd == -1)
		{
			netfd=fd;
			INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] destroy \n", _sid, _cid);
			CSessionMgr::Instance()->set_in_process_con(NULL);
			CSessionMgr::Instance()->destroy_session(_owner->_id);
			return -1;
		}

		set_con_state(CON_STATE_BUSY);
		CSessionMgr::Instance()->set_in_process_con(NULL);
		return 0;

	}
	else if (_owner->get_session_type() == SESSION_DATA)
	{

		if (_send_que.size() <= 0)
		{
			DisableOutput();
			return 0;
		}

		while(_send_que.size() > 0) {
		_total_send = _send_que.front()._data_len;
		char* send_buf = _send_que.front()._data;
		int ret=0;

		if(_owner->_subtype == GeneralSession::SUB_STYPE_TCP|| _owner->_subtype == GeneralSession::SUB_STYPE_TCP_MULTI_CON)
		{
			ret = send(netfd, send_buf + _have_sent, _total_send - _have_sent, 0);
		}
		else
		{
			struct sockaddr_in addr;
			bzero(&addr, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr=_com_info.remoteip_;
			addr.sin_port = htons(_owner->_port);
			socklen_t socklen =sizeof(addr);
			ret = sendto(netfd, send_buf + _have_sent, _total_send - _have_sent, 0,(sockaddr*)&addr,socklen);
		}
		if (-1 == ret)
		{
			if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
			{
				EnableOutput();
				ApplyEvents();
				continue;
			}

			//对于长连接出错应该重连
			//DisableInput ();
			//DisableOutput ();
			INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] OutputNotify send error: %m \n", _sid, _cid);
			ret = handle_event(EVENT_SHUTDOWN_COM);

			if (ret == CON_QUIT)
				goto OUTPUT_DESTROY_CON;

			ret = do_connect();

			if (ret != SPP_ASYNC::ASYNC_SUCC)
				goto OUTPUT_DESTROY_CON;
			continue;
		}

		_have_sent += ret;

		if (_have_sent == _total_send)
		{
			_have_sent = 0;
			_total_send = 0;
			handle_event(EVENT_SEND_SUCC);
			continue;
		}

		if (_have_sent < _total_send)
		{
			EnableOutput();
			ApplyEvents();
			continue;
		}
	} // end while(_send_que.size() > 0)
	} // end if (_owner->get_session_type() == SESSION_DATA)

	set_con_state(CON_STATE_BUSY);
	CSessionMgr::Instance()->set_in_process_con(NULL);
	return 0;

OUTPUT_DESTROY_CON:
	CSessionMgr::Instance()->set_in_process_con(NULL);
	delete this;
	return -1;
}

int CClientUnit::HangupNotify(void)
{
	if (_io_state == CONN_CONNECTING) {
		_io_state = CONN_DISCONNECT;
		DetachPoller();
		return 0;
	}

	INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] HangupNotify \n", _sid, _cid);
	CSessionMgr::Instance()->set_in_process_con(this);
	set_con_state(CON_STATE_IN_PROCESS);
	update_idle_timer();

	if (_owner->get_session_type() == SESSION_CONTROL) {
		void* server_base = CSessionMgr::Instance()->get_serverbase();

		void* param = _owner->_fd_exception_param;

		if (_owner->_fd_exception_f == NULL)
			return CON_SUCC;

		int fd=netfd;
		_owner->_fd_exception_f(_owner->_id, &fd, param, server_base);

		if (_logic_state == CON_STATE_TERMINATE)
			goto HANGUP_DESTROY_CON;

		if(fd==-1)
		{
			netfd=fd;
		}
		INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] destroy \n", _sid, _cid);
		CSessionMgr::Instance()->set_in_process_con(NULL);
		CSessionMgr::Instance()->destroy_session(_owner->_id);
		return -1;
	} 
	else if (_owner->get_session_type() == SESSION_DATA)
	{
		int ret = handle_event(EVENT_SHUTDOWN_COM);

		if (ret == CON_QUIT)
			goto HANGUP_DESTROY_CON;
		else {

			ret = do_connect();

			if (ret != SPP_ASYNC::ASYNC_SUCC)
				goto HANGUP_DESTROY_CON;

			CSessionMgr::Instance()->set_in_process_con(NULL);
			return 0;
		}
	}

HANGUP_DESTROY_CON:

	CSessionMgr::Instance()->set_in_process_con(NULL);
	delete this;
	return -1;
}


void CClientUnit::TimerNotify(void)
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d,%d] TimerNotify\n", _sid, _cid);

    if (_io_state != CONN_CONNECTED) {
        do_connect();
        return ;
    }

    CSessionMgr::Instance()->set_in_process_con(this);
    set_con_state(CON_STATE_IN_PROCESS);
    int ret = handle_event(EVENT_TIMEOUT);

    if (ret == CON_QUIT)
        goto TIMEOUT_DESTROY_CON;
    else {
        ret = do_connect();

        if (ret != SPP_ASYNC::ASYNC_SUCC)
            goto TIMEOUT_DESTROY_CON;

        CSessionMgr::Instance()->set_in_process_con(NULL);
        return ;
    }

TIMEOUT_DESTROY_CON:
    CSessionMgr::Instance()->set_in_process_con(NULL);
    delete this;
}

void CClientUnit::update_idle_timer(void)
{
    
    DisableTimer();
    if(_owner->_idle_timer!=NULL)
    {
    	AttachTimer(_owner->_idle_timer);
    }
    else if (CSessionMgr::Instance()->get_global_idletimer()!=NULL)
    {
	AttachTimer(CSessionMgr::Instance()->get_global_idletimer());	
    }
}

void CClientUnit::update_reconnect_timer(void)
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] update_reconnect_timer\n", _sid, _cid);
    DisableTimer();
    AttachTimer(CSessionMgr::Instance()->get_reconnect_timer());
}


void CClientUnit::clear_send_buf()
{
    char* p = NULL;

    for (int i = _send_que.size(); i > 0; i--) {
        if ((p = _send_que.front()._data) != NULL)
            free(p);

        _send_que.pop();
    }

    _have_sent = 0;
    _total_send = 0;
}

void CClientUnit::reset()
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d:%d] reset\n", _sid, _cid);
    clear_send_buf();
    DetachPoller();
    DisableTimer();
    _event = EVENT_NORMAL;
    _logic_state = CON_STATE_IDLE;
    _io_state = CONN_IDLE;
    _recv_cnt = 0;
    _local_input_param = NULL;
    _local_proc_param = NULL;
    _cache->skip(_cache->data_len());
}

void CClientUnit::recycle()
{
    INTERNAL_LOG->LOG_P(LOG_DEBUG, "cu[%d,%d] recycle\n", _sid, _cid);
    reset();
    _owner->recycle_con(_cid);
    set_con_state(CON_STATE_IDLE);
    Attach();
}
