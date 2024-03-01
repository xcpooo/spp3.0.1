#include "monitor.h"

namespace spp
{

namespace comm
{

CMonitorBase *_spp_g_monitor = NULL;

CSngMonitor::CSngMonitor()
{
	//用来判断是否支持 monitorc
	//449476 spp启动 用来探测机器是否支持monitor
	if (sng_attr_api::Attr_API(449476, 1) < 0)
	{
		throw runtime_error("unsupport sng monitor");
	}
}

}
}
