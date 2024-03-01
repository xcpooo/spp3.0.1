#ifndef __SESSION_MGR_H__
#define	__SESSION_MGR_H__

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <noncopyable.h>
#include "client_unit.h"
#include "../comm/tbase/tsockcommu/tmempool.h"
#include "../comm/serverbase.h"
#include "timerlist.h"
#include "../comm/singleton.h"
#include "spp_async_interface.h"
#include "general_session.h"
#include "tm_session.h"

namespace ASYNC_SPP_II
{

	class CSessionMgr : private noncopyable
	{
		public:
			static CSessionMgr* Instance(void);
			CSessionMgr(): _isEnableAsync(false), _pollerunit(NULL), _timerunit(NULL), _globalidletimer(NULL),
                _reconnect_timerlist(NULL), _server_base(NULL),	_in_process_con(NULL), 
                _epwaittimeout(1000), _free_s_id(MIN_SESSION_ID), _free_tms_id(MIN_SESSION_ID),
                _max_epoll_timeout(DEFAULT_EPOLL_WAIT_TIME) {
            }

			~CSessionMgr();

			int init_mgr();

			int close(void);

			int run(bool isBlock);

			int init(string config_file, int max_epoll_timeout, int TOS);

            int init_notify(int key);
            inline int disable_notify(void);
            inline int enable_notify(void);

			int create_session(const int sid, const char* type, const char* subtype,
					const char* ip, const int port, const int recv_cnt, const int timeout,
					const int inf = DEFAULT_MULTI_CON_INF, const int sup = DEFAULT_MULTI_CON_SUP);

			//第二套接口调用
			int create_session(const int sid, int fd, raw_handle_func input_f = NULL, void* input_param = NULL,
					raw_handle_func output_f = NULL, void* output_param = NULL,
					raw_handle_func exception_f = NULL, void* exception_param = NULL);
			//end

			int create_tm_session(const int sid, const int cb_interval, void* cb_param, time_task_func cb_func);

			int send_data(int sid,  char* buf, int len, void* input_param = NULL, void* proc_param = NULL, void* pack_info = NULL);

			int reg_session_callBack(int sid, process_func proc, void* proc_param, input_func input_f, void* input_param);

			//第二套接口调用
			int reg_session_callBack(const int sid, raw_handle_func input_f, void* input_param,
					raw_handle_func output_f, void* output_param,
					raw_handle_func exception_f, void* exception_param);

			int EnableInput(const int sid);

			int DisableInput(const int sid);

			int EnableOutput(const int sid);

			int DisableOutput(const int sid);
			//end

			int reg_pack_callBack(int sid, pack_cb_func proc, void* global_param);


			int destroy_session(const int sid);

			int destroy_tm_session(const int sid);

			int recycle_con(int sid = -1, int con_id = -1);

			int add_send_buf(int sid, char* buf, int len);


			int  set_session_idle_timer(int sid,int time); 

			inline CTimerList* get_reconnect_timer()    {
				return _reconnect_timerlist;
			}

			void   set_reconnect_timer(int interval = DEFAULT_RECONNECT_INTERVAL);

			//	int   set_con_idle_timer(int timeout=DEFAULT_CON_IDLE_TIMEOUT);


			inline CTimerList* get_session_timer(int time_limit) {
				return _timerunit->GetTimerList(time_limit);
			}

			inline CTimerList* get_global_idletimer()
			{
				return _globalidletimer;
			}

			inline CPollerUnit* get_pollerunit(void) {
                _isEnableAsync = true; // 新异步会调
				return _pollerunit;
			}

			inline CTimerUnit* get_timerunit(void) {
				return _timerunit;
			}

			inline void	set_in_process_con(CClientUnit* c) {
				_in_process_con = c;
			}

			inline CClientUnit*	get_in_process_con() {
				return _in_process_con;
			}

			inline CMemPool&    get_mempool() {
				return _mem_pool;
			}

			inline void set_serverbase(spp::comm::CServerBase* server_base) {
				_server_base = server_base;
			}

			inline spp::comm::CServerBase*  get_serverbase() {
				return  _server_base;
			}

			typedef std::map<int, GeneralSession*> GS_MAP;
			typedef std::map<int, TmSession*> TMS_MAP;
			typedef std::map<int, void*>	USER_DATA_MAP;
			typedef GS_MAP::iterator GS_ITE;
			typedef TMS_MAP::iterator TMS_ITE;

			void	set_user_data(int flow, void* data);
			void*	get_user_data(int flow);

			// Run timerunit, check expired 
			void check_expired();

            bool    _isEnableAsync;			
            bool    _isEnableTimer;

		protected:
            bool check_reserve_sid(int id);
            bool check_reserve_tm_sid(int id);
            int  alloc_sid();
            int  alloc_tm_sid();
            int  re_connect(int, int*);

            const static int ERR_SESSION_ID = 0;
            const static int MAX_SESSION_ID = 0x7FFFFFFF;
            const static int MIN_SESSION_ID = (RESERVED_SESSION_ID_SUP + 1);
            const static int MAX_NET_TRANSFER_DELAY_US = 50000;
            const static int DEFAULT_CONNECT_RETRY_INTERVAL = 10000;

            GS_MAP                                  _SessionMap;
            GS_MAP                                  _UnConnectedSessionMap;	//unused
            TMS_MAP                                 _TmSessionMap;
            USER_DATA_MAP                           _user_data_map;
            CMemPool     							_mem_pool;
            CPollerUnit*                            _pollerunit;
            CTimerUnit*                             _timerunit;
            CTimerUnit*                             _reconnect_timerunit;
            CTimerList*                             _globalidletimer;
            CTimerList*                             _reconnect_timerlist;
            spp::comm::CServerBase*                 _server_base;
            CClientUnit*                            _in_process_con;
            
            int                                     _epwaittimeout;
            int                                     _free_s_id;
            int                                     _free_tms_id;

            int     _max_epoll_timeout;
			int     _TOS;
    };

}
#endif
