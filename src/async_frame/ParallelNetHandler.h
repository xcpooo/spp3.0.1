/************************************************************
    FileName   ParallelNetHandler.h
    Author     kylexiao
    Version    1.0
    Create     2011/09/30
    DESC:      多发多收NetHandler
    History
        <author>    <time>      <version >  <desc>

    Copyright:  Tencent, China
*************************************************************/

#ifndef __SPP_PARALLEL_NET_HANDLER_H__
#define __SPP_PARALLEL_NET_HANDLER_H__

#include "CommDef.h"
#include "poller.h"
#include "timerlist.h"
#include <string>
#include <list>
#include <vector>
#include <sys/time.h>

BEGIN_ASYNCFRAME_NS

class CActionInfo;

class CParallelNetHandler
    : public CPollerObject
{
    public:
        enum ConnState {
            UNCONNECTED = 0,
            CONNECTING,
            CONNECTED,
        };

		class CActionContext : public CTimerObject
		{
		public:
            CActionContext(CParallelNetHandler* ptr)
            :owner(ptr) {}

			~CActionContext()
			{
				DisableTimer();
			}
			void TimerNotify(void)
			{
				owner->TimerNotify(seq);
			}
			CParallelNetHandler * owner;
			CActionInfo * pAI;
			uint64_t seq;
			uint64_t start_time;
		};

		typedef struct
		{
			uint64_t    seq;
			char*       buf;
			int         len;
			int         sent;
			CActionContext* ac;
		}send_elem_t;


    public:
        CParallelNetHandler(CPollerUnit *pollerUnit,
                        CTimerUnit *timerUnit,
                        const std::string &ip,
                        PortType port,
                        ProtoType proto,
                        ConnType conn_type,
						int TOS);

        virtual ~CParallelNetHandler();

        // 处理请求
        virtual int HandleRequest(CActionInfo *ai);

        // 获取目标IP
        inline void GetDestIP(std::string &ip) {
            ip = _dest_ip;
        }

        // 获取目标port
        inline void GetDestPort(PortType &port) {
            port = _dest_port;
        }

        // 获取协议类型
        inline void GetProtoType(ProtoType &proto) {
            proto = _proto;
        }

        // 获取连接类型
        inline void GetConnType(ConnType &type) {
            type = _conn_type;
        }
		
		inline bool IsHeathy()
        {
            return _timeout_cnt == 0;
        }

        inline unsigned int GetActionCnt()
        {
            return _sending_cnt + _recving_cnt;
        }

    protected:
        int SetNonBlocking(int fd);
        int Open();
        int Close();
        int DoConnect();
        int DoSend();
        int DoRecv();
        int DoProcess();
        void DoRecvFinished(const char* buf, int len, uint64_t seq);
        void DoErrorFinished(int err);
        void CopyToPending(send_elem_t* se);
        void NotifyAction(uint64_t seq, int result, CActionInfo *ai, int timecost);
        void UpdateRouteResultIfNeed(int ecode, CActionInfo * action);

    public: // CPollerObject
        virtual int InputNotify();
        virtual int OutputNotify();
        virtual int HangupNotify();

        void TimerNotify(uint64_t seq);

    protected:
        std::string                 _dest_ip;
        PortType                    _dest_port;
        struct sockaddr_in          _dest_addr;
        ProtoType                   _proto;
        ConnType                    _conn_type;

        unsigned int                _sending_cnt;
		std::list<send_elem_t*>     _sending_list;
		std::vector<send_elem_t*>   _sending_elem_pool; //pool

        unsigned int                _recving_cnt;
		std::list<CActionContext*>  _recving_list;
		std::vector<CActionContext*> _recving_elem_pool;

        CTimerUnit *                _timerUnit;

        ConnState                   _conn_state;

		int							_timeout_cnt;

        char *                      _s_pending_buf;
        int                         _s_pending_size;

		char *						_r_buf;
		int							_r_buf_size;
		int							_r_data_len;

		int                         _TOS;

		bool                        _log_trace_on;
};

END_ASYNCFRAME_NS

#endif
