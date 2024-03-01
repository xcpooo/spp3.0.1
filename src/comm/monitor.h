#ifndef __SPP_MONITOR_H__
#define __SPP_MONITOR_H__

#include "attr_api/Attr_API.h"

#include<stdexcept>

using namespace std;
using namespace sng_attr_api;

namespace spp
{
namespace comm
{
#define MONITOR_PROXY_DOWN 231168 //spp_proxy进程down，重新拉起
#define MONITOR_WORKER_DOWN 231169 //spp_worker进程down，重新拉起
#define MONITOR_PROXY_MORE 231170 //spp_proxy进程太多，减少进程
#define MONITOR_WORKER_MORE 231539 //spp_worker进程太多，减少进程
#define MONITOR_PROXY_DT 231557 //spp_proxy进程处于D/T状态，无法拉起
#define MONITOR_WORKER_DT 231558 //spp_worker进程处于D/T状态，无法拉起
#define MONITOR_PROXY_LESS 231559 //spp_proxy进程数不足，增加进程
#define MONITOR_WORKER_LESS 231560 //spp_worker进程数不足，增加进程
#define MONITOR_WORKER_DOWN2 281854 //spp_worker down
#define MONITOR_PROXY_RECV_UDP 269535	 //proxy收到udp包 
#define MONITOR_PROXY_DROP_UDP 269536	 //proxy因为过载丢弃的udp请求 
#define MONITOR_PROXY_PROC_UDP 269537	 //proxy处理的udp包 
#define MONITOR_PROXY_ACCEPT_TCP 269538	 //proxy收到TCP新建连接 
#define MONITOR_PROXY_REJECT_TCP 269539	 //proxy因为过载拒绝TCP连接 
#define MONITOR_PROXY_ACCEPT_TCP_SUSS 269540	 //proxy新建TCP连接成功 
#define MONITOR_TIMER_CLEAN_CONN 269541	 //定期清理连接数量 
#define MONITOR_CONNSET_NEW_CONN 269542	 //ConnSet新增连接数 
#define MONITOR_CONNSET_CLOSE_CONN 269543	 //ConnSet关闭连接数 
#define MONITOR_PROXY_PROC_TCP 269544	 //proxy处理TCP请求数 
#define MONITOR_CLIENT_CLOSE_TCP 269545	 //客户端主动关闭TCP连接 
#define MONITOR_PROXY_PROC 269546	 //proxy处理请求数 
#define MONITOR_PROXY_TO_WORKER 269547	 //proxy发送到worker的请求数 
#define MONITOR_WORKER_TO_PROXY 269548	 //proxy收到worker回复请求数 
#define MONITOR_CONNSET_TIMER_CHECK 269549	 //ConnSet定期检查连接是否过期 
#define MONITOR_WORKER_FROM_PROXY 269550	 //worker收到proxy请求数 
#define MONITOR_WORKER_OVERLOAD_DROP 269551	 //worker防雪崩丢球请求数 
#define MONITOR_WORKER_PROC_SUSS 269552	 //worker spp_handle_process处理成功数 
#define MONITOR_WORKER_PROC_FAIL 269553	 //worker spp_handle_process处理失败数 
#define MONITOR_WORKER_RECV_DELAY_1 269554	 //worker从队列取包延迟[0~1]ms 
#define MONITOR_WORKER_RECV_DELAY_10 269559	 //worker从队列取包延迟(1~10]ms 
#define MONITOR_WORKER_RECV_DELAY_50 269560	 //worker从队列取包延迟(10~50]ms 
#define MONITOR_WORKER_RECV_DELAY_100 269637	 //worker从队列取包延迟(50~100]ms 
#define MONITOR_WORKER_RECV_DELAY_XXX 269638	 //worker从队列取包延迟100+ms 
#define MONITOR_PROXY_RELAY_DELAY_1 269639	 //proxy回包延时[0~1]ms 
#define MONITOR_PROXY_RELAY_DELAY_10 269640	 //proxy回包延时(1~10]ms 
#define MONITOR_PROXY_RELAY_DELAY_50 269641	 //proxy回包延时(10~50]ms 
#define MONITOR_PROXY_RELAY_DELAY_100 269642	 //proxy回包延时(50~100]ms 
#define MONITOR_PROXY_RELAY_DELAY_XXX 269643	 //proxy回包延时100+ms 
#define MONITOR_CLOSE_UDP 288337	 //关闭Udp链接数 
#define MONITOR_CLOSE_TCP 288338	 //关闭TCP连接数 
#define MONITOR_TIMEOUT_UDP 288340	 //清理超时UDP连接数 
#define MONITOR_TIMEOUT_TCP 288341	 //清理超时TCP连接数 
#define MONITOR_MT_START 320833	 //微线程处理启动 
#define MONITOR_MT_OVER 320834	 //微线程处理完毕 
#define MONITOR_MT_ERRMSG 320835	 //微线程消息非法 
#define MONITOR_MT_MSGFAIL 320836	 //微线程消息失败 
#define MONITOR_MT_CREATE_FAIL 320837	 //微线程创建失败 
#define MONITOR_MT_MEMORY_LESS 320838	 //微线程内存不足 
#define MONITOR_MT_BIZ_ENCODE_FAIL 320839	 //微线程业务组包失败 
#define MONITOR_MT_STATE_PROC_FAIL 320840	 //微线程状态处理失败 
#define MONITOR_MT_COOKIE_ERR 320841	 //微线程cookie非法 
#define MONITOR_MT_SOCKET_FAIL 320842	 //微线程socket创建失败 
#define MONITOR_MT_CONNECT_FAIL 320843	 //微线程connect失败 
#define MONITOR_MT_SEND_FAIL 320844	 //微线程发送失败 
#define MONITOR_MT_RECV_TIMEOUT 320845	 //微线程接收应答包超时或失败 
#define MONITOR_MT_POOL_LESS 320846	 //微线程池过小 
#define MONITOR_MT_POOL_CREATE_FAIL 320847	 //微线程池创建失败 
#define MONITOR_MT_HEAP_INSERT_FAIL 320848	 //微线程堆插入失败 
#define MONITOR_MT_HEAP_REMOVE_FAIL 320849	 //微线程堆删除失败 
#define MONITOR_MT_EPOLL_CTL_FAIL 320850	 //微线程epollctrl失败*/ 
#define MONITOR_MT_EPOLL_FD_ERR 320851	 //微线程epoll fd非法 
#define MONITOR_OVERLOAD_DISABLED 344278	 //防雪崩超时时间为0，防雪崩较果没开。异常ID2 
#define MONITOR_UDP_SESSION_DELAY 350403	 //UDP会话session延迟到达 
#define MONITOR_MT_RECV 350778	 //微线程收到包 
#define MONITOR_MT_SEND 350779	 //微线程发送包 
#define MONITOR_MT_OVERLOAD 361140	 //微线程创建达到最大并发上限 
#define MONITOR_WORKER_SOCKET_FAIL 389902	 //worker socket创建失败 
#define MONITOR_WORKER_SEND_FAIL 389903	 //worker socket发送失败 
#define MONITOR_WOKER_CONNECT_FAIL 389904	 //worker socket连接失败 
#define MONITOR_SEND_FLOWID_ERR 410934	 //应答消息flowid异常 
#define MONITOR_RECV_FLOWID_ERR 410935	 //接收消息flowid异常 
#define MONITOR_CLOSE_FLOWID_ERR 410936	 //关闭连接时flowid异常 


#define AsyncFrame_Error_Begin            2145127
#define AsyncFrame_EConnect_1             2145127
#define AsyncFrame_ESend_2                2145128
#define AsyncFrame_ERecv_3                2145129
#define AsyncFrame_ECheckPkg_4            2145130
#define AsyncFrame_ETimeout_5             2145131
#define AsyncFrame_ECreateSock_6          2145132
#define AsyncFrame_EAttachPoller_7        2145133   
#define AsyncFrame_EInvalidState_8        2145134
#define AsyncFrame_EHangup_9              2145135
#define AsyncFrame_EShutdown_10           2145136
#define AsyncFrame_EEncodeFail_11         2145137
#define AsyncFrame_EInvalidRouteType_12   2145138
#define AsyncFrame_EMsgTimeout_13         2145139 
#define AsyncFrame_EGetRouteFail_14       2145140 
#define AsyncFrame_ERecvUncomplete_15     2145141
#define AsyncFrame_EOverLinkErrLimit_16   2145142
#define AsyncFrame_EGetRouteOverload_17   2145143
#define AsyncFrame_Error_End                        2145143

#define AsyncFrame_Error_Others   2145249



#define MONITOR(iAttrId) do { \
	if (_spp_g_monitor) { \
		 _spp_g_monitor->report(iAttrId, 1); \
	} \
} while(0);

#define CUSTOM_MONITOR(iAttrId, iValue) do { \
	if (_spp_g_monitor) { \
		_spp_g_monitor->report(iAttrId, iValue); \
	} \
} while(0);

#define MONITOR_SET(iAttrId, iValue) do { \
	if (_spp_g_monitor) { \
		_spp_g_monitor->set(iAttrId, iValue); \
	} \
} while(0);


class CMonitorBase
{
public:
	CMonitorBase(){}
	virtual ~CMonitorBase(){}

	virtual void report(const int iAttrId, int iValue = 1) = 0;	
	virtual void set(const int iAttrId, int iValue = 1) = 0;
};

class CSngMonitor : public CMonitorBase
{
public:
	CSngMonitor();
	virtual ~CSngMonitor(){}

	virtual void report(const int iAttrId, int iValue = 1)
	{
		sng_attr_api::Attr_API(iAttrId, iValue);
	}
	
	virtual void set(const int iAttrId, int iValue = 1)
	{
		sng_attr_api::Attr_API_Set(iAttrId, iValue);
	}
	
};

extern CMonitorBase *_spp_g_monitor;

}
}

#endif
