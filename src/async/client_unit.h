#ifndef __CLIENT_SYNC_H__
#define __CLIENT_SYNC_H__

#include <queue>
#include "global.h"
#include <noncopyable.h>
#include "spp_async_def.h"
#include "poller.h"
#include "timerlist.h"
#include "general_session.h"
#include"../comm/tbase/tsockcommu/tcache.h"
#include"../comm/serverbase.h"
#include"../comm/tbase/tcommu.h"
#include"general_session.h"


namespace ASYNC_SPP_II
{

    typedef struct pack_blob {
        pack_blob(char* data, int data_len, void* cb_info): _data(data), _data_len(data_len), _cb_info(cb_info) {
        }
        pack_blob(const pack_blob& blob): _data(blob._data), _data_len(blob._data_len), _cb_info(blob._cb_info) {
        }
        char* _data;
        int   _data_len;
        void* _cb_info;

    } PACK_BLOB;
	

    class CClientUnit : public CPollerObject, public CTimerObject, private noncopyable
    {
    public:
        CClientUnit(GeneralSession* s, int fd, int cid, int TOS);

        virtual ~CClientUnit();

        int Attach(void);
        inline void set_io_state(CConnState state) {
            _io_state = state;
        }
        inline CConnState get_io_state(void) const {
            return _io_state;
        }
        inline void set_netfd(int fd) {
            close(netfd);
            netfd = fd;
        }
        inline int get_netfd(void) const  {
            return netfd;
        }
        inline GeneralSession* get_owner() {
            return _owner;
        }
        inline int get_cid()   {
            return _cid;
        }
        inline int get_sid() const  {
            return _owner->_id;
        }
        inline bool is_long_connect() {
            return _owner->_long_connect;
        }
        inline void set_con_state(CON_STATE state) {
            _logic_state = state;
        }
        inline CON_STATE get_con_state() const {
            return _logic_state;
        }
        inline void set_event(SESSION_EVENT e) {
            _event = e;
        }
        inline SESSION_EVENT get_event() const {
            return _event;
        }
        inline bool	 is_overaccess() const {
            return _owner->_recv_count <= 0 ? false : _recv_cnt >= _owner->_recv_count ? true : false;

        }
        inline void set_local_input_param(void* param) {
            _local_input_param = param;
        }
        inline void set_local_proc_param(void* param) {
            _local_proc_param = param;
        }
        virtual int OutputNotify(void);
        int AddSendBuf(char* buf, int len, void* pack_info);
        virtual int InputNotify(void);
        virtual int HangupNotify(void);
        virtual void TimerNotify(void);
        void update_idle_timer(void);
        void update_reconnect_timer(void);
        void clear_send_buf();

        int handle_recv(int& recv_len);
        int handle_input(int& msg_len);
        int handle_proc(int proc_len);
        int	handle_pack_cb(SESSION_EVENT e, PACK_BLOB& pack);

        int handle_event(SESSION_EVENT e);
        int do_connect(int timeout=0);

        void  reset();
        void  recycle();

    private:
        const static int    						MAX_NET_TRANSFER_DELAY_US = 50000;
        CConnState        							_io_state;
        CON_STATE	      							_logic_state;
        std::queue<PACK_BLOB> 						_send_que;
        int 										_sid;
        int 										_cid;
        int 										_have_sent;
        int 										_total_send;
        GeneralSession* 							_owner;
        tbase::tcommu::tsockcommu::CRawCache*       _cache;
        SESSION_EVENT   							_event;
        int 										_recv_cnt;
        void*										_local_input_param;
        void*										_local_proc_param;
        tbase::tcommu::TConnExtInfo					_com_info;
		int                                         _TOS;
    };
}
#endif

