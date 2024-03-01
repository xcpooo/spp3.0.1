/**
 *  @file SyncFrame.cpp
 *  @info ͬ���߳�״̬���ʵ��
 *  @time 20130515
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>

#include "tlog.h"
#include "tcommu.h"
#include "serverbase.h"
#include "CommDef.h"
#include "comm_def.h"
#include "frame.h"
#include "AsyncFrame.h" 
#include "defaultworker.h"
#include "IFrameStat.h"
#include "StatComDef.h"

#include "mt_msg.h"
#include "mt_version.h"
#include "micro_thread.h"

#include "SyncMsg.h"
#include "SyncFrame.h"
#include "monitor.h"

USING_ASYNCFRAME_NS;

using namespace std;
using namespace spp::worker;
using namespace spp::statdef;
using namespace NS_MICRO_THREAD;
using namespace SPP_SYNCFRAME;

namespace SPP_SYNCFRAME {
/**
 * @brief SPP��־����΢�߳̿�ܶ��岿��
 */
#define LOG_MAX_LEN     4096
class SppLogAdpt : public LogAdapter
{
private:
    CServerBase *_base;                    ///< spp�ķ���������
    char        _buff[LOG_MAX_LEN];        ///< ��־��ʱ����
    
public:

    /**
     * @brief ����������
     */
    SppLogAdpt(CServerBase* base)  :  _base(base) {
    }; 
    virtual ~SppLogAdpt(){};

    /**
     * @brief ��־������˼��ӿ�ʵ��
     */
    bool CheckLogLvl(LOG_LEVEL lvl) {
        if (!_base) { 
            return false;
        }
        
        int iLogLvl = _base->log_.log_level(-1); 
        if (lvl >= iLogLvl) {
            return true;
        } else {
            return false;
        }
    }

    virtual bool CheckTrace() {
        return CheckLogLvl(LOG_TRACE); //  lvl 1
    };    
    virtual bool CheckDebug() {
        return CheckLogLvl(LOG_DEBUG);  //  lvl 2
    };
    virtual bool CheckError() { 
        return CheckLogLvl(LOG_ERROR);  //  lvl 3
    };
    

    /**
     * @brief DEBUG�ӿ�ʵ��
     */
    virtual void LogDebug(char* fmt, ...){
        if (_base) {            
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);          
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_DEBUG, "%s\n", _buff);
        }
    };

    /**
     * @brief TRACE�ӿ�ʵ��
     */
    virtual void LogTrace(char* fmt, ...){
        if (_base) {        
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);          
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_NORMAL, "%s\n", _buff);
        }
    };

    /**
     * @brief ERROR�ӿ�ʵ��
     */
    virtual void LogError(char* fmt, ...){           
        if (_base) { 
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);  
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_ERROR, "%s\n", _buff);
        }
    }; 

    /**
     * @brief attr�ӿ�ʵ��
     */
    virtual void AttrReportAdd(int attr, int iValue) {
    	CUSTOM_MONITOR(attr, iValue);
    };


	/**
     * @brief attr�ӿ�ʵ��
     */
    virtual void AttrReportSet(int attr, int iValue) {
    	MONITOR_SET(attr, iValue);
		
		if (_base) {
			CDefaultWorker* worker = (CDefaultWorker*)_base;
			switch(attr)
			{			
				case 492069://΢�̳߳ش�С						
					worker->fstat_.op(WIDX_MT_THREAD_NUM, iValue);
					break;
				case 493212://΢�߳���Ϣ��				
					worker->fstat_.op(WIDX_MT_MSG_NUM, iValue);
					break;
			}
    	}
    };
};




/**
 * �߳�ͳһ��ں���, �������Ϣ�Ĵ�����
 *
 * @param pMsg CMsgBase���������ָ�룬��ź�������ص����ݣ��ö�����Ҫ�����new�ķ�ʽ���䣬�ͷ��ɿ�ܸ���
 *
 * @return 0: �ɹ��� ������ʧ��
 *
 */ 
void ThreadEntryFunc(void* arg)
{
	CSyncFrame::Instance()->_uMsgNo++;
    MT_ATTR_API_SET(493212, CSyncFrame::Instance()->_uMsgNo); // ΢�߳���Ϣ��

    MT_ATTR_API(320833, 1); // ����
    
    int rc = 0;
    CSyncMsg* msg = (CSyncMsg*)arg;
    if (!msg)
    {
        MT_ATTR_API(320835, 1); // ��Ϣ�Ƿ�
        SF_LOG(LOG_ERROR, "Invalid arg, error");
        goto EXIT_LABEL;
    }

    rc = msg->HandleProcess();
    if (rc != 0)
    { 
        MT_ATTR_API(320836, 1); // ��Ϣʧ��
        SF_LOG(LOG_DEBUG, "User msg process failed(%d), close connset", rc);
        
        blob_type rspblob;        
        rspblob.len = 0;
        rspblob.data = NULL;
        msg->SendToClient(rspblob);
    }

EXIT_LABEL:

    if (arg) {		
		CSyncFrame::Instance()->_uMsgNo--;
		STEP_REQ_STAT(msg->GetMsgCost());
        delete msg;
    } 

    MT_ATTR_API(320834, 1); // ���

    return;
};


/**
 *  ժ��SPP��notify����, ʡȥ�ļ�����
 */
int SppShmNotify(int key)
{
    if(key <= 0) {
        return -1;
    }
    
    int netfd = -1;
    char path[32] = {0};
    snprintf(path, sizeof(path), ".notify_%d", key);

    if(mkfifo(path, 0666) < 0) {
        if (errno != EEXIST) {
            printf("create fifo[%s] error[%m]\n", path);
            return -2;
        } 
    }

    netfd = open(path, O_RDWR | O_NONBLOCK, 0666);
    if(netfd == -1) {
        printf("open notify fifo[%s] error[%m]\n", path);
        return -3;
    }

    return netfd;
}


/**
 *  @brief  ͬ����ܲ��ȫ�ֳ�Ա��ʼ��
 */
CSyncFrame *CSyncFrame::_s_instance = NULL;
bool CSyncFrame::_init_flag  = false;
int  CSyncFrame::_iNtfySock  = -1;


/**
 *  @brief  ͬ����ܲ��ȫ�־����ȡ
 */
CSyncFrame* CSyncFrame::Instance (void)
{
    if( NULL == _s_instance )
    {
        _s_instance = new CSyncFrame;
    }

    return _s_instance;
}


/**
 *  @brief  ͬ����ܲ������
 */
CSyncFrame::CSyncFrame() 
{
    _pServBase = NULL;
    _iGroupId  = 0;
    _iNtfyFd   = -1;
	_uMsgNo    = 0;
}

/**
 *  @brief  ͬ����ܲ������
 */
CSyncFrame::~CSyncFrame() 
{ 
    if (_iNtfyFd != -1)
    {
        close(_iNtfyFd);
        _iNtfyFd = -1;
    }
}

/**
 *  @brief  ͬ����ܲ������ʼ��
 */
void CSyncFrame::Destroy (void)
{
    if (!CSyncFrame::_init_flag)
    {
        return;
    }
    
    CAsyncFrame::Instance()->FiniFrame();
    CAsyncFrame::Destroy();

    MtFrame::Instance()->Destroy();
    
    if( _s_instance != NULL )
    {
        delete _s_instance;
        _s_instance = NULL;
    }

    CSyncFrame::_init_flag = false;

    return;
}

/**
 *  @brief  ͬ����ܲ����ʼ��
 */
int CSyncFrame::InitFrame(CServerBase *pServBase, int max_thread_num, int min_thread_num)
{
    if (CSyncFrame::_init_flag)
    {
        SF_LOG(LOG_ERROR, "CSyncFrame already init ok");
        return 0;
    }
    CSyncFrame::_init_flag = true;  // ͬ���̱߳��λ
    
    _pServBase = pServBase;    
    CAsyncFrame::Instance()->InitFrame2(_pServBase); ///< ����ACTIONinfo ��Ҫ

    ///< ��ʼ��MT��
    static SppLogAdpt s_log(_pServBase);
    MtFrame::Instance()->SetDefaultThreadNum(min_thread_num);
    bool rc = MtFrame::Instance()->InitFrame(&s_log, max_thread_num);
    if (!rc)
    {
        SF_LOG(LOG_ERROR, "InitFrame sync failed; rc %d", rc);
        return -2;
    }

    /// < ����realtime��־
    MtFrame::Instance()->SetRealTime(_spp_g_exceptionrealtime);

    ///< ȡGROUPID, ���������ܵ�, ��ȡ�ɶ��¼�֪ͨ 20130523
    _iNtfyFd = SppShmNotify(_iGroupId*2); // notify comsume
    int flags;
    if ((flags = fcntl(_iNtfyFd, F_GETFL, 0)) < 0 || 
          fcntl(_iNtfyFd, F_SETFL, flags | O_NONBLOCK) < 0) 
    {
        close(_iNtfyFd);
        _iNtfyFd = -1;
        SF_LOG(LOG_ERROR, "set non block failed; rc %d", rc);
        return -2;
    }
    
    MtFrame::sleep(0);
    return 0;
}


/**
 * ͬ����Ϣ�ĵ������
 *
 * @param pMsg CMsgBase���������ָ�룬��ź�������ص����ݣ��ö�����Ҫ�����new�ķ�ʽ���䣬�ͷ��ɿ�ܸ���
 *
 * @return 0: �ɹ��� ������ʧ��
 *
 */ 
int CSyncFrame::Process(CSyncMsg *pMsg)
{
    if (MtFrame::CreateThread(ThreadEntryFunc, pMsg) == NULL) 
    {
        MT_ATTR_API(320837, 1); // ����ʧ��
        SF_LOG(LOG_ERROR, "Sync frame start thread failed, error");
        delete pMsg;
        return -1;
    }
    return 0;
};

/**
 *  ���߳�ѭ��, ÿ���л�������, �ó�CPU, ���ȿ�ܺ��epoll
 */
void CSyncFrame::WaitNotifyFd(int utime)
{
    static char szbuf[1024];
    if (CSyncFrame::_iNtfySock != -1)
    {
        MtFrame::WaitEvents(CSyncFrame::_iNtfySock, EPOLLIN, utime);
    }
    else
    {
        MtFrame::read(_iNtfyFd, szbuf, 1024, utime);
    }
}


/**
 *  ΢�߳�������־, SPPע���ڲ��ӿ�
 */
bool CSyncFrame::GetMtFlag()
{
    return CSyncFrame::_init_flag;
}

/**
 *  ���߳�ѭ��, ����ǰ���Ƿ�������, ����epoll�ȴ�����
 */
void CSyncFrame::HandleSwitch(bool block)
{
    static int loopcnt = 0;

    if (block)
    {
        CSyncFrame::Instance()->WaitNotifyFd(10);
    }
    else
    {
        loopcnt++;
        int run_cnt = MtFrame::Instance()->RunWaitNum();
        if ((run_cnt >= 10) || (loopcnt % 100 == 0))
        {
            MtFrame::sleep(0);
        }    
    }
}

const std::string CSyncFrame::GetCurrentMsg()
{
	std::string strReq = "";

    MicroThread *thread = MtFrame::Instance()->GetRootThread();
    if (thread)
    {
	    CSyncMsg *msg = (CSyncMsg *)thread->GetThreadArgs();
	    if (msg)
	    {
		    strReq = msg->GetReqPkg();
	    }
    }

	return strReq;
}

bool CSyncFrame::CheckCurrentStack(long lESP)
{
	char *esp = (char *)lESP;
	
	MicroThread *thread = MtFrame::Instance()->GetActiveThread();
    if (thread)
    {
		return  thread->CheckStackHealth(esp);
    }

	return true;
}

int CSyncFrame::GetThreadNum()
{
	return MtFrame::Instance()->GetUsedNum();
}

void CSyncFrame::SetGroupId(int id)
{
    _iGroupId = id;
}

void CSyncFrame::sleep(int ms)
{
	MtFrame::Instance()->sleep(ms);
}

}
