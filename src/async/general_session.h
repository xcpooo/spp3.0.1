#ifndef __GENERAL_SESSION_H__
#define	__GENERAL_SESSION_H__

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "spp_async_def.h"
#include "noncopyable.h"
#include "timerlist.h"
#include "asyn_api.h"
#include "interface.h"
#include "../comm/singleton.h"
#include "spp_async_interface.h"

namespace  ASYNC_SPP_II
{

    class CClientUnit;

    class  GeneralSession: public CTimerObject
    {
    public:
        typedef std::map<int , CClientUnit*> CON_MAP;
        enum Main_SType {
            MAIN_STYPE_TTC = 0,
            MAIN_STYPE_TDB,
            MAIN_STYPE_CUSTOM,
            MAIN_STYPE_UNKNOWN
        };

        enum Sub_SType {
            SUB_STYPE_TCP = 0,
            SUB_STYPE_UDP,
            SUB_STYPE_TCP_MULTI_CON,
            SUB_STYPE_UNKNOWN
        };


        GeneralSession(const int sid, const char* type, const char* subtype, const char* ip, const int port,
                       const int recv_cnt, const int timeout, const int multi_con_inf = DEFAULT_MULTI_CON_INF,
                       const int multi_con_sup = DEFAULT_MULTI_CON_SUP);

		//add by jeremy
		GeneralSession(const int sid, int fd, raw_handle_func input_f = NULL, void* input_param = NULL,
				       raw_handle_func output_f = NULL, void* output_param = NULL,
					   raw_handle_func exception_f = NULL, void* exception_param = NULL);

        ~GeneralSession();

        inline bool check() {
            if (_maintype == MAIN_STYPE_UNKNOWN || _subtype == SUB_STYPE_UNKNOWN) {
                return false;
            }

            return true;
        }

        virtual void TimerNotify(void);

		// not used
        int add_con(int con_id, int fd);

        int init_cons(int TOS);

		//add by jeremy
		int init_cons(int fd, int TOS);

        void recycle_con(int con_id);

        void remove_con(int con_id);

        int send_data(char* buf, int buf_len, void* input_param = NULL, void* proc_param = NULL, void* pack_info = NULL);

        void reg_callback(recv_func recv_f, void * recv_param, process_func proc_f,
                          void* proc_param, input_func input_f, void* input_param) {
            _recv_f = recv_f;
            _input_f = input_f;
            _proc_f = proc_f;
            _recv_param = recv_param;
            _input_param = input_param;
            _proc_param = proc_param;
		}

		//add by jeremy
		void reg_callback(raw_handle_func input_f, void* input_param,
				          raw_handle_func output_f, void* output_param,
						  raw_handle_func exception_f, void* exception_param)
		{
			_fd_input_f = input_f;
			_fd_input_param = input_param;
			
			_fd_output_f = output_f;
			_fd_output_param = output_param;

			_fd_exception_f = exception_f;
			_fd_exception_param = exception_param;

		}

        void reg_pack_callback(pack_cb_func proc, void* global_param) {
            _pack_cb_global_param = global_param;
            _pack_cb_f = proc;
        }

        void Attach();

        void add_active_con(CClientUnit* con);
        void del_active_con(CClientUnit* con);

		CClientUnit* get_client_unit(){
			return _mainCon;
		}

		SESSION_TYPE get_session_type(){
			return _session_type;
		}

    protected:
        int 	   		_id;
        Main_SType 		_maintype;
        Sub_SType   	_subtype;
        SESSION_STATUS	_status;
        std::string		    	_ip;
        int 	   		_port;
        bool 	   		_long_connect;
        int 	   		_recv_count;
        int 	   		_timeout;
        int 	   		_multi_con_inf;
        int 	   		_multi_con_sup;
        CClientUnit* 	_mainCon;
        std::set<int>* 	_idle_con_set;
        CON_MAP	   		*_con_map;

        recv_func       _recv_f;
        input_func      _input_f;
        process_func    _proc_f;
        pack_cb_func	_pack_cb_f;
        void			*_recv_param;
        void			*_input_param;
        void			*_proc_param;
        void			*_pack_cb_global_param;

        CTimerList*	_idle_timer;
        friend class 	CClientUnit;
        friend class 	CSessionMgr;

		//支持第二套接口的数据成员
		raw_handle_func	_fd_input_f;
		raw_handle_func	_fd_output_f;
		raw_handle_func	_fd_exception_f;
		void			*_fd_input_param;
		void			*_fd_output_param;
		void			*_fd_exception_param;
		int				_fd;

		SESSION_TYPE	_session_type;
		int             _TOS;
    };

}
#endif
