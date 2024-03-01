#include"client_unit.h"
#include"session_mgr.h"
#include"spp_async_interface.h"
#include"timerlist.h"

using namespace ASYNC_SPP_II;

SPP_ASYNC::SID SPP_ASYNC::CreateSession(const int sid, const char* type, const char* subtype, const char* ip, const int  port,
                                        const int recv_cnt, const int timeout, const int multi_con_inf, const int multi_con_sup)
{
    return CSessionMgr::Instance()->create_session(sid, type, subtype, ip, port, recv_cnt, timeout, multi_con_inf, multi_con_sup);
}

//第二套接口
SPP_ASYNC::SID SPP_ASYNC::CreateSession(const int sid, int fd, raw_handle_func input_f, void* input_param,
		                                 raw_handle_func output_f, void* output_param,
										 raw_handle_func exception_f, void* exception_param)
{
	return CSessionMgr::Instance()->create_session(sid, fd, input_f, input_param, output_f, output_param, 
												   exception_f, exception_param);
}
//end

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC:: DestroySession(SID sid)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->destroy_session(sid);
}

SPP_ASYNC::SID  SPP_ASYNC:: CreateTmSession(const int tm_sid, const int time_interval, time_task_func func, void* task_para)
{
    return CSessionMgr::Instance()->create_tm_session(tm_sid, time_interval, task_para, func);
}


SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::DestroyTmSession(SID tm_sid)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->destroy_tm_session(tm_sid);
}

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::RegSessionCallBack(SID sid, process_func proc_f, void* proc_param,
                                                            input_func input_f, void* input_param)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->reg_session_callBack(sid, proc_f, proc_param, input_f, input_param);

}

//第二套接口
SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::RegSessionCallBack(const int sid, raw_handle_func input_f, void* input_param,
		                                          			raw_handle_func output_f, void* output_param,
												            raw_handle_func exception_f, void* exception_param)
{
	return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->reg_session_callBack(sid, input_f, input_param, output_f, output_param,
																			 exception_f, exception_param);
}

int SPP_ASYNC::EnableInput(const int sid)
{
	return CSessionMgr::Instance()->EnableInput(sid);
}

int SPP_ASYNC::DisableInput(const int sid)
{
	return CSessionMgr::Instance()->DisableInput(sid);
}

int SPP_ASYNC::EnableOutput(const int sid)
{
	return CSessionMgr::Instance()->EnableOutput(sid);
}

int SPP_ASYNC::DisableOutput(const int sid)
{
	return CSessionMgr::Instance()->DisableOutput(sid);
}
//end

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::RegPackCallBack(SID sid, pack_cb_func proc, void* global_param)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->reg_pack_callBack(sid, proc, global_param);
}


SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::SendData(SID sid, char* buf, int buflen)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->send_data(sid, buf, buflen);
}

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::SendData(SID sid, char* buf, int buflen, void* pack_info)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->send_data(sid, buf, buflen, NULL, NULL, pack_info);
}


SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::SendData(SID sid, char* buf, int buflen, void* input_param, void* proc_param)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->send_data(sid, buf, buflen, input_param, proc_param);
}

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::SendData(SID sid, char* buf, int buflen, void* input_param, void* proc_param, void* pack_info)
{
    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->send_data(sid, buf, buflen, input_param, proc_param, pack_info);
}

SPP_ASYNC::SPP_ASYNC_RET_ENUM SPP_ASYNC::RecycleCon(SID sid, CONID conid)
{

    return (SPP_ASYNC_RET_ENUM)CSessionMgr::Instance()->recycle_con(sid, conid);
}

void   SPP_ASYNC::SetConnectRetryInterval(int interval)
{
    CSessionMgr::Instance()->set_reconnect_timer(interval);
}

void   SPP_ASYNC::SetIdleTimeout(int timeout,int sid)
{
   CSessionMgr::Instance()->set_session_idle_timer(timeout,sid);
}

void	SPP_ASYNC::SetUserData(int flow, void* data)
{

    CSessionMgr::Instance()->set_user_data(flow, data);
}

void*   SPP_ASYNC::GetUserData(int flow)
{
    return CSessionMgr::Instance()->get_user_data(flow);
}


CPollerUnit* SPP_ASYNC::GetPollerUnit()
{
    return CSessionMgr::Instance()->get_pollerunit();
}

CTimerUnit* SPP_ASYNC::GetTimerUnit()
{
    return CSessionMgr::Instance()->get_timerunit();
}
