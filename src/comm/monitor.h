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
#define MONITOR_PROXY_DOWN 231168 //spp_proxy����down����������
#define MONITOR_WORKER_DOWN 231169 //spp_worker����down����������
#define MONITOR_PROXY_MORE 231170 //spp_proxy����̫�࣬���ٽ���
#define MONITOR_WORKER_MORE 231539 //spp_worker����̫�࣬���ٽ���
#define MONITOR_PROXY_DT 231557 //spp_proxy���̴���D/T״̬���޷�����
#define MONITOR_WORKER_DT 231558 //spp_worker���̴���D/T״̬���޷�����
#define MONITOR_PROXY_LESS 231559 //spp_proxy���������㣬���ӽ���
#define MONITOR_WORKER_LESS 231560 //spp_worker���������㣬���ӽ���
#define MONITOR_WORKER_DOWN2 281854 //spp_worker down
#define MONITOR_PROXY_RECV_UDP 269535	 //proxy�յ�udp�� 
#define MONITOR_PROXY_DROP_UDP 269536	 //proxy��Ϊ���ض�����udp���� 
#define MONITOR_PROXY_PROC_UDP 269537	 //proxy�����udp�� 
#define MONITOR_PROXY_ACCEPT_TCP 269538	 //proxy�յ�TCP�½����� 
#define MONITOR_PROXY_REJECT_TCP 269539	 //proxy��Ϊ���ؾܾ�TCP���� 
#define MONITOR_PROXY_ACCEPT_TCP_SUSS 269540	 //proxy�½�TCP���ӳɹ� 
#define MONITOR_TIMER_CLEAN_CONN 269541	 //���������������� 
#define MONITOR_CONNSET_NEW_CONN 269542	 //ConnSet���������� 
#define MONITOR_CONNSET_CLOSE_CONN 269543	 //ConnSet�ر������� 
#define MONITOR_PROXY_PROC_TCP 269544	 //proxy����TCP������ 
#define MONITOR_CLIENT_CLOSE_TCP 269545	 //�ͻ��������ر�TCP���� 
#define MONITOR_PROXY_PROC 269546	 //proxy���������� 
#define MONITOR_PROXY_TO_WORKER 269547	 //proxy���͵�worker�������� 
#define MONITOR_WORKER_TO_PROXY 269548	 //proxy�յ�worker�ظ������� 
#define MONITOR_CONNSET_TIMER_CHECK 269549	 //ConnSet���ڼ�������Ƿ���� 
#define MONITOR_WORKER_FROM_PROXY 269550	 //worker�յ�proxy������ 
#define MONITOR_WORKER_OVERLOAD_DROP 269551	 //worker��ѩ������������ 
#define MONITOR_WORKER_PROC_SUSS 269552	 //worker spp_handle_process����ɹ��� 
#define MONITOR_WORKER_PROC_FAIL 269553	 //worker spp_handle_process����ʧ���� 
#define MONITOR_WORKER_RECV_DELAY_1 269554	 //worker�Ӷ���ȡ���ӳ�[0~1]ms 
#define MONITOR_WORKER_RECV_DELAY_10 269559	 //worker�Ӷ���ȡ���ӳ�(1~10]ms 
#define MONITOR_WORKER_RECV_DELAY_50 269560	 //worker�Ӷ���ȡ���ӳ�(10~50]ms 
#define MONITOR_WORKER_RECV_DELAY_100 269637	 //worker�Ӷ���ȡ���ӳ�(50~100]ms 
#define MONITOR_WORKER_RECV_DELAY_XXX 269638	 //worker�Ӷ���ȡ���ӳ�100+ms 
#define MONITOR_PROXY_RELAY_DELAY_1 269639	 //proxy�ذ���ʱ[0~1]ms 
#define MONITOR_PROXY_RELAY_DELAY_10 269640	 //proxy�ذ���ʱ(1~10]ms 
#define MONITOR_PROXY_RELAY_DELAY_50 269641	 //proxy�ذ���ʱ(10~50]ms 
#define MONITOR_PROXY_RELAY_DELAY_100 269642	 //proxy�ذ���ʱ(50~100]ms 
#define MONITOR_PROXY_RELAY_DELAY_XXX 269643	 //proxy�ذ���ʱ100+ms 
#define MONITOR_CLOSE_UDP 288337	 //�ر�Udp������ 
#define MONITOR_CLOSE_TCP 288338	 //�ر�TCP������ 
#define MONITOR_TIMEOUT_UDP 288340	 //����ʱUDP������ 
#define MONITOR_TIMEOUT_TCP 288341	 //����ʱTCP������ 
#define MONITOR_MT_START 320833	 //΢�̴߳������� 
#define MONITOR_MT_OVER 320834	 //΢�̴߳������ 
#define MONITOR_MT_ERRMSG 320835	 //΢�߳���Ϣ�Ƿ� 
#define MONITOR_MT_MSGFAIL 320836	 //΢�߳���Ϣʧ�� 
#define MONITOR_MT_CREATE_FAIL 320837	 //΢�̴߳���ʧ�� 
#define MONITOR_MT_MEMORY_LESS 320838	 //΢�߳��ڴ治�� 
#define MONITOR_MT_BIZ_ENCODE_FAIL 320839	 //΢�߳�ҵ�����ʧ�� 
#define MONITOR_MT_STATE_PROC_FAIL 320840	 //΢�߳�״̬����ʧ�� 
#define MONITOR_MT_COOKIE_ERR 320841	 //΢�߳�cookie�Ƿ� 
#define MONITOR_MT_SOCKET_FAIL 320842	 //΢�߳�socket����ʧ�� 
#define MONITOR_MT_CONNECT_FAIL 320843	 //΢�߳�connectʧ�� 
#define MONITOR_MT_SEND_FAIL 320844	 //΢�̷߳���ʧ�� 
#define MONITOR_MT_RECV_TIMEOUT 320845	 //΢�߳̽���Ӧ�����ʱ��ʧ�� 
#define MONITOR_MT_POOL_LESS 320846	 //΢�̳߳ع�С 
#define MONITOR_MT_POOL_CREATE_FAIL 320847	 //΢�̳߳ش���ʧ�� 
#define MONITOR_MT_HEAP_INSERT_FAIL 320848	 //΢�̶߳Ѳ���ʧ�� 
#define MONITOR_MT_HEAP_REMOVE_FAIL 320849	 //΢�̶߳�ɾ��ʧ�� 
#define MONITOR_MT_EPOLL_CTL_FAIL 320850	 //΢�߳�epollctrlʧ��*/ 
#define MONITOR_MT_EPOLL_FD_ERR 320851	 //΢�߳�epoll fd�Ƿ� 
#define MONITOR_OVERLOAD_DISABLED 344278	 //��ѩ����ʱʱ��Ϊ0����ѩ���Ϲ�û�����쳣ID2 
#define MONITOR_UDP_SESSION_DELAY 350403	 //UDP�Ựsession�ӳٵ��� 
#define MONITOR_MT_RECV 350778	 //΢�߳��յ��� 
#define MONITOR_MT_SEND 350779	 //΢�̷߳��Ͱ� 
#define MONITOR_MT_OVERLOAD 361140	 //΢�̴߳����ﵽ��󲢷����� 
#define MONITOR_WORKER_SOCKET_FAIL 389902	 //worker socket����ʧ�� 
#define MONITOR_WORKER_SEND_FAIL 389903	 //worker socket����ʧ�� 
#define MONITOR_WOKER_CONNECT_FAIL 389904	 //worker socket����ʧ�� 
#define MONITOR_SEND_FLOWID_ERR 410934	 //Ӧ����Ϣflowid�쳣 
#define MONITOR_RECV_FLOWID_ERR 410935	 //������Ϣflowid�쳣 
#define MONITOR_CLOSE_FLOWID_ERR 410936	 //�ر�����ʱflowid�쳣 


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
