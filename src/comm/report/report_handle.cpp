#include "report_handle.h"
#include "loop_queue.h"
#include "report_base.h"

#include <string.h>
#include <stdio.h>

static report_handle_t g_report_handle;

CReport::CReport()
{
	g_report_handle.bitmaps = 0;
	g_report_handle.msg_cost = 0;
	memset(g_report_handle.cmd, 0 , sizeof(g_report_handle.cmd));
}

CReport::~CReport()
{
}

void CReport::set_cmd(const char *cmd)
{
	int srclen = strlen(cmd);
	int dstlen = sizeof(g_report_handle.cmd) - 1;

	g_report_handle.bitmaps |= REPORT_FIELD_CMD;
	strncpy((char *)&g_report_handle.cmd[0], cmd, srclen >= dstlen ? dstlen : srclen);
}

void CReport::set_result(int32_t result)
{
	g_report_handle.bitmaps |= REPORT_FIELD_RESULT;
	g_report_handle.result = result;
}

void CReport::set_local_addr(uint32_t ip, uint32_t port)
{
	g_report_handle.bitmaps |= REPORT_FIELD_LOCAL;
	g_report_handle.local_ipv4 = ip;
	g_report_handle.local_port = port;
}

void CReport::set_remote_addr(uint32_t ip, uint32_t port)
{
	g_report_handle.bitmaps |= REPORT_FIELD_REMOTE;
	g_report_handle.remote_ipv4 = ip;
	g_report_handle.remote_port = port;
}

void CReport::set_msg_cost(uint32_t cost)
{
	g_report_handle.bitmaps |= REPORT_FIELD_COST;
	g_report_handle.msg_cost = cost;
}

void CReport::set_usr_define(uint32_t cookie)
{
	g_report_handle.bitmaps |= REPORT_FIELD_USR_DEF;
	g_report_handle.usr_define = cookie;
}

int CReport::do_report()
{
	int iRet = -5;
	
	if (
		(g_report_handle.bitmaps & REPORT_FIELD_CMD) > 0
		&& (g_report_handle.bitmaps & REPORT_FIELD_RESULT) > 0
		)
	{		
		iRet = CLoopQueue::Instance()->push(&g_report_handle, sizeof(g_report_handle));
		if (-2 == iRet)
		{
			printf("loop queue not init\n");
			CLoopQueue::Instance()->init(REPORTFILE, MAXNUMBER, sizeof(g_report_handle));
			iRet = CLoopQueue::Instance()->push(&g_report_handle, sizeof(g_report_handle));
		}
		
		g_report_handle.bitmaps = 0;
		g_report_handle.msg_cost = 0;
	}
	
	return iRet;
}

int CReport::proc_report(void *data, struct timeval *tv)
{
	int iRet = -1;
	
	iRet = CLoopQueue::Instance()->pop(data, sizeof(g_report_handle));
	if (-2 == iRet)
	{
		printf("loop queue not init\n");
		CLoopQueue::Instance()->init(REPORTFILE, MAXNUMBER, sizeof(g_report_handle));
		iRet = CLoopQueue::Instance()->pop(data, sizeof(g_report_handle), tv);
	}

	return iRet;
}

