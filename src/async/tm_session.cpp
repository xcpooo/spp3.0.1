#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "timerlist.h"
#include<stdio.h>
#include "session_mgr.h"
#include"../comm/singleton.h"


using namespace std;
using namespace tbase::tcommu;
using namespace ASYNC_SPP_II;
using namespace::tbase::tlog;
using namespace::spp::singleton;




void TmSession::TimerNotify(void)
{
    void* server_base = CSessionMgr::Instance()->get_serverbase();
    SetStatus(SESSION_INPROCESS);
    _cb_func(_id, _cb_param, server_base);

    if (_status == SESSION_TERMINATED) {
        delete this;
        return;
    }
    AttachTimer(CSessionMgr::Instance()->get_session_timer(_cb_interval));
    SetStatus(SESSION_IDLE);
}
