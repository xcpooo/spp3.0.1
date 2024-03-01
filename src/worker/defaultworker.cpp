#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include "defaultworker.h"
#include "misc.h"
#include "benchadapter.h"
#include "misc.h"
#include "global.h"
#include "singleton.h"

#include "comm_def.h"

#include "../comm/tconfbase/tinyxmlcomm.h"
#include "../comm/tconfbase/workerconf.h"
#include "../comm/keygen.h"
#include "../comm/tbase/list.h"
#include "../comm/tbase/notify.h"
#include "../comm/tbase/tsockcommu/tsocket.h"
#include "../comm/exception.h"
#include "../comm/monitor.h"
#include "../async/session_mgr.h"
#include "../async_frame/AsyncFrame.h"

#include "../nest/comm/nest_commu.h"

#include "StatMgr.h"
#include "StatMgrInstance.h"

#include "SyncFrame.h"

#define WORKER_STAT_BUF_SIZE 1<<14

#define MAX_FILENAME_LEN 255

using namespace spp;
using namespace spp::comm;
using namespace spp::worker;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace spp::exception;
using namespace std;
using namespace ASYNC_SPP_II;
using namespace tbase::notify;
using namespace spp::tconfbase;
using namespace nest;
using namespace SPP_SYNCFRAME;
USING_ASYNCFRAME_NS;

USING_SPP_STAT_NS;

extern CoreCheckPoint g_check_point;
extern struct sigaction g_sa;
extern void sigsegv_handler(int sig);
extern struct timeval __spp_g_now_tv; // session_mgr.cpp
extern int g_spp_L5us;                // NetHandler.cpp 2.10.8 release 2
int g_spp_groupid;
int g_spp_groups_sum;

const std::string get_msg()
{
    if (((CDefaultWorker*)g_worker)->get_mt_flag())
    {
		return CSyncFrame::Instance()->GetCurrentMsg();
    }

	return "";
}

bool check_stack(long lESP)
{
    if (((CDefaultWorker*)g_worker)->get_mt_flag())
    {
		return CSyncFrame::Instance()->CheckCurrentStack(lESP);
    }

	return true;
}

CDefaultWorker::CDefaultWorker(): ator_(NULL),
    TOS_(-1), commu_local_(0), notify_fd_(0)
{
    __spp_do_update_tv();
    __spp_get_now_ms();
}

CDefaultWorker::~CDefaultWorker()
{
    if (ator_ != NULL)
        delete ator_;

}

// 获取是否启用微线程
bool CDefaultWorker::get_mt_flag()
{
    // 静态库和动态库版本的微线程同时存在，哪个版本有做初始化，则使用了微线程。
    bool flag = CSyncFrame::Instance()->GetMtFlag();
    if (flag)
    {
        
        return true;
    }

    if (sppdll.spp_mirco_thread && sppdll.spp_mirco_thread())
    {
        return true;
    }

    return false;
}

// 微线程切换函数
void CDefaultWorker::handle_switch(bool block)
{
    static CSyncFrame* sync_frame = CSyncFrame::Instance();
    bool flag = sync_frame->GetMtFlag();
    
    // 1. local commu need check notify change
    if (commu_local_) 
    {
        static TNetCommuMng* net_mng = TNetCommuMng::Instance();
        int32_t commu_fd = net_mng->GetClientFd();
        if (notify_fd_ != commu_fd) // notify fd updated
        {
            notify_fd_ = commu_fd;       
            if (flag) // frame dll so
            {
                CSyncFrame::_iNtfySock = notify_fd_;
            }
            else if (sppdll.spp_set_notify) // service so
            {
                sppdll.spp_set_notify(notify_fd_);    
            }
        }
    }

    // 2. switch to run micro-thread
    if (flag)  // 使用了动态库版本的微线程
    {
        sync_frame->HandleSwitch(block);
    }
    else if(sppdll.spp_handle_switch) // 使用了静态库版本的微线程
    {
        sppdll.spp_handle_switch(block);
    }
}

void CDefaultWorker::realrun(int argc, char* argv[])
{
    //初始化配置

    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    CSessionMgr::Instance()->init_mgr();
    CSessionMgr::Instance()->set_serverbase(this);

    initconf(false);

    time_t nowtime = 0, montime = 0;
    int64_t now_ms = 0, check_ms = 0;

    flog_.LOG_P_PID(LOG_FATAL, "worker started!\n");

    ///< start: check sync frame init flag 20130715
    bool mt_flag = false;
    if (get_mt_flag())
    {
        mt_flag = true;
    }
    ///< end: check sync frame init flag 20130715

    __spp_do_update_tv();

    while (true)
    {
        ///< start: micro thread handle loop entry add 20130715
        if (mt_flag && sppdll.spp_handle_loop)
        {
            sppdll.spp_handle_loop(this);   
        }            
        ///< end: micro thread handle loop entry 20130715

        // == 0 时，表示没取到请求，进入较长时间异步等待
        bool isBlock = (ator_->poll(false) == 0);

        if (mt_flag) ///< micro thread run flag 20130715
        {
            CSessionMgr::Instance()->check_expired();
            handle_switch(isBlock);

            if (commu_local_)
            {
                TNestNetComm::Instance()->RunPollUnit(0); // rcv commu data
            }
        }
        else
        {
            CSessionMgr::Instance()->run(isBlock);
        }
        // Check and reconnect net proxy, default 10 ms 
        now_ms = __spp_get_now_ms();
        if ((commu_local_ == 1) && ((now_ms - check_ms) > 10))
        {
            check_ms = now_ms;
            TNetCommuMng::Instance()->CheckChannel();
        }

        //检查quit信号
        if (unlikely(CServerBase::quit()) || unlikely(CServerBase::reload()))
        {
	        __spp_do_update_tv();
			now_ms = __spp_get_now_ms();
			
            // 保证剩下的请求都处理完
            if (unlikely(CServerBase::quit()))
            {        
	            flog_.LOG_P_PID(LOG_FATAL, "recv quit signal at %u\n", now_ms);
            	ator_->poll(true);
            }
			else
			{			
				flog_.LOG_P_PID(LOG_FATAL, "recv reload signal at %u\n", now_ms);
			}

			int timeout = 0;
			if (CSessionMgr::Instance()->_isEnableAsync)
			{
				//异步框架
	            while (CAsyncFrame::Instance()->GetMsgCount() > 0 && timeout < 1000)
	            {            	
					CSessionMgr::Instance()->run(false);
	                usleep(10000);
					timeout += 10;
	            }
			}

			if (mt_flag)
			{
				//微线程
				while (CSyncFrame::Instance()->GetThreadNum() > 1 && timeout < 1000)
				{
					CSessionMgr::Instance()->check_expired();
	                CSyncFrame::Instance()->sleep(10000);
					timeout += 10;
				}
			}
			
	        __spp_do_update_tv();
			now_ms = __spp_get_now_ms();
			flog_.LOG_P_PID(LOG_FATAL, "exit at %u\n", now_ms);
			break;
        }

        //监控信息上报
        nowtime = __spp_get_now_s();

        if ( unlikely(nowtime - montime > ix_->moni_inter_time_) )
        {
            CLI_SEND_INFO(&moncli_)->timestamp_ = nowtime;
            moncli_.run();
            montime = nowtime;
            flog_.LOG_P_PID(LOG_DEBUG, "moncli run!\n");
            
            // add check msg ctx clean, max timeout 5m 
            if (commu_local_ != 0) {
                TNetCommuMng::Instance()->CheckMsgCtx(__spp_get_now_ms(), 5*60*1000);
            }
        }

    }

    g_check_point = CoreCheckPoint_HandleFini;           // 设置待调用插件的CheckPoint
    if (sppdll.spp_handle_fini != NULL)
        sppdll.spp_handle_fini(NULL, this);
    g_check_point = CoreCheckPoint_SppFrame;                // 恢复CheckPoint，重置为CoreCheckPoint_SppFrame

    CStatMgrInstance::destroy();

    flog_.LOG_P_PID(LOG_FATAL, "worker stopped!\n");
}

void CDefaultWorker::assign_signal(int signo)
{
	switch (signo) {
		case SIGSEGV:
		case SIGFPE:
		case SIGILL:
		case SIGABRT:
		case SIGBUS:
		case SIGSYS:
		case SIGTRAP:
			signal(signo, sigsegv_handler);
			break;
		default:
			exit(1);
	}
}

int CDefaultWorker::initconf(bool reload)
{
    if ( reload )
    {
        return 0;
    }

    int pid = getpid();

    printf("\nWorker[%5d] init...\n", pid);

    set_serverbase(this);

    int ret = 0;

    CWorkerConf conf(new CLoadXML(ix_->argv_[1]));

    ret = conf.init();
    if (ret != 0)
    {
        exit(-1);
    }

	//加载coredump & memlog 配置
	Exception ex;
	if (conf.getAndCheckException(ex) == 0)
	{
		_spp_g_exceptioncoredump = ex.coredump;
		_spp_g_exceptionpacketdump = ex.packetdump;
		_spp_g_exceptionrestart = ex.restart;
		_spp_g_exceptionmonitor = ex.monitor;
		_spp_g_exceptionrealtime = ex.realtime;
	}

	if (!_spp_g_exceptionrestart)
	{
		//处理SIGSEGV信号，在coredump时记录日志
        assign_signal(SIGSEGV);
        assign_signal(SIGFPE);
        assign_signal(SIGILL);
        assign_signal(SIGABRT);
        assign_signal(SIGBUS);
        assign_signal(SIGSYS);
        assign_signal(SIGTRAP);
	}
	
	if (_spp_g_exceptionmonitor)
	{
		//初始化monitor
		try
		{
			_spp_g_monitor = new CSngMonitor;
		}
		catch(runtime_error &ex)
		{
			_spp_g_monitor = NULL;
		}
	}

	
    Worker worker = conf.getWorker();

    groupid_ = worker.groupid;
    TOS_ = worker.TOS;
    g_spp_groupid = groupid_;
    g_spp_groups_sum = groupid_;
    g_spp_L5us = worker.L5us;
    g_spp_shm_fifo = worker.shm_fifo;

    // add global config info
    spp_global::link_timeout_limit = worker.link_timeout_limit; 
    if (0 == spp_global::link_timeout_limit)
    {
        spp_global::link_timeout_limit = 100; // > continued 100 timeout, close parallel link 
    }

    printf("Worker[%5d] Groupid = %d  L5us = %d shm_fifo = %d\n", pid, groupid_, worker.L5us, worker.shm_fifo);

    CSyncFrame::Instance()->SetGroupId(groupid_);

    //初始化框架日志
    Log flog;
    if (conf.getAndCheckFlog(flog) == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    flog_.LOG_OPEN(flog.level, flog.type, flog.path.c_str(),
                           flog.prefix.c_str(), flog.maxfilesize, flog.maxfilenum);

    //初始化业务日志
    Log log;
    if (conf.getAndCheckLog(log) == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    log_.LOG_OPEN(log.level, log.type, log.path.c_str(), log.prefix.c_str(), log.maxfilesize, log.maxfilenum);


    int notifyfd;
    spp_global::mem_full_warning_script = "";
    notifyfd = CNotify::notify_init( groupid_*2 + 1 ); // notify produce

    if (notifyfd < 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]producer notify.groupid[%d]%m\n", groupid_);
        exit(-1);
    }

    ret = CSessionMgr::Instance()->init_notify( groupid_*2 ); // notify comsume
    if (ret < 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]comsumer notify.groupid[%d]%m\n", groupid_);
        exit(-1);
    }

    TShmCommuConf shm;
    shm.shmkey_producer_ = pwdtok(groupid_ * 2 + 1);
    shm.shmkey_comsumer_ = pwdtok(groupid_ * 2);
    shm.locktype_ = LOCK_TYPE_PRODUCER | LOCK_TYPE_COMSUMER;
    shm.notifyfd_ = notifyfd;

    ConnectShm acceptor;
    conf.getAndCheckAcceptor(acceptor);

    spp_global::mem_full_warning_script = acceptor.scriptname;
    shm.shmsize_producer_ = acceptor.entry[0].send_size * (1 << 20);
    shm.shmsize_comsumer_ = acceptor.entry[0].recv_size * (1 << 20) ;
    //  改变了语义，从2.7.0起，超时不在共享内存队列里进行
    msg_timeout_ = acceptor.entry[0].msg_timeout;

    if(msg_timeout_==0)
    {
        MONITOR(MONITOR_OVERLOAD_DISABLED);
    }

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] [Shm]Proxy->Worker [%dMB]\n", pid, shm.shmsize_comsumer_/(1<<20));
    LOG_SCREEN(LOG_ERROR, "Worker[%5d] [Shm]Worker->Proxy [%dMB]\n", pid, shm.shmsize_producer_/(1<<20));

    // check commu type
    if (acceptor.entry[0].type != "local")
    {
        ator_ = new CTShmCommu;
        ret = ator_->init(&shm);

        if (ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "[ERROR]Worker acceptor init error, return %d\n", ret);
            exit(-1);
        }
    }
    else
    {
        // 1. Init set poller unit
        bool async_flag = CSessionMgr::Instance()->_isEnableAsync;
        CPollerUnit* poller = CSessionMgr::Instance()->get_pollerunit();
        CSessionMgr::Instance()->_isEnableAsync = async_flag;
        TNestNetComm::Instance()->SetPollUnit(poller);
        
        // 2. disp regist recv function
        TCommuCltDisp& disp = TNetCommuMng::Instance()->GetCommuCltDisp();
        disp.reg_cb(CB_RECVDATA, ator_recvdata_v2, this);

        // 3. Init commu obj
        ator_ = new TNetCommuClient;
        if (!ator_)
        {
            LOG_SCREEN(LOG_ERROR, "[ERROR]Worker acceptor init error, return %d\n", ret);
            exit(-1);
        }
        TNetCommuMng::Instance()->RegistClient((TNetCommuClient*)ator_);

        // 4. Set connect address
        struct sockaddr_un uaddr = {0};
        uaddr.sun_family   = AF_UNIX;
        snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), "../bin/.unix_socket");
        
        nest::TNetCoummuConf net_conf;
        net_conf.route_id       = groupid_;
        net_conf.address.SetAddress(&uaddr);
        if (ator_->init(&net_conf) < 0)
        {
            flog_.LOG_P_PID(LOG_ERROR, "[ERROR] connect local error, type unix, wait retry!!!\n");
        }

        // 5. Set local commu flag
        commu_local_ = 1;
    }

    ator_->reg_cb(CB_SENDDATA, ator_senddata, this);
    ator_->reg_cb(CB_OVERLOAD, ator_overload, this);
    ator_->reg_cb(CB_SENDERROR, ator_senderror, this);

    if (spp_global::mem_full_warning_script != "")
    {
        struct stat fstat;

        if (stat(spp_global::mem_full_warning_script.c_str(), &fstat) == -1 ||
                (fstat.st_mode & S_IFREG) == 0)
        {
            fprintf( stderr, "[WARNING] %d Attribute scriptname error in config file %s.\n",  __LINE__, ix_->argv_[1]);
            spp_global::mem_full_warning_script.clear();
        }
    }


    //初始化框架统计
    Stat fstat;
    conf.getAndCheckFStat(fstat);

    int stat_ret = fstat_.init_statpool(fstat.file.c_str());
    if (stat_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error, ret:%d, errmsg:%m\n", stat_ret);
        exit(-1);
    }

    fstat_.init_statobj_frame(SHM_RX_BYTES, STAT_TYPE_SUM, WIDX_SHM_RX_BYTES,
            "接收包量/字节数");
    fstat_.init_statobj_frame(SHM_TX_BYTES, STAT_TYPE_SUM, WIDX_SHM_TX_BYTES,
            "发送包量/字节数");
    fstat_.init_statobj_frame(MSG_TIMEOUT, STAT_TYPE_SUM, WIDX_MSG_TIMEOUT,
            "Proxy请求丢弃数");
    fstat_.init_statobj_frame(MSG_SHM_TIME, STAT_TYPE_SUM, WIDX_MSG_SHM_TIME,
            "proxy->worker 请求接收到开始处理时延/毫秒");

    fstat_.init_statobj_frame(MT_THREAD_NUM, STAT_TYPE_SET, WIDX_MT_THREAD_NUM,
            "微线程数量");
    fstat_.init_statobj_frame(MT_MSG_NUM, STAT_TYPE_SET, WIDX_MT_MSG_NUM,
            "微线程消息数");

    //初始化业务统计
    Stat stat;
    conf.getAndCheckStat(stat);

    if (stat_.init_statpool(stat.file.c_str()) != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error.\n");
        exit(-1);
    }

    //初始化监控

    Moni moni;
    if (conf.getAndCheckMoni(moni) == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }
    ix_->moni_inter_time_ = moni.intervial;


    // 初始化与Ctrl通信的MessageQueue
    int mqkey = pwdtok(255);
    if (mqkey == 0)
    {
        mqkey = DEFAULT_MQ_KEY;
    }

    CCommu* commu = new CMQCommu(mqkey);
    moncli_.set_commu(commu);
    memset(CLI_SEND_INFO(&moncli_), 0x0, sizeof(TProcInfo));
    CLI_SEND_INFO(&moncli_)->groupid_ = groupid_;
    CLI_SEND_INFO(&moncli_)->procid_ = getpid();
    CLI_SEND_INFO(&moncli_)->timestamp_ = time(NULL);
    CLI_SEND_INFO(&moncli_)->reserve_[0] = 1;//资源尚未就绪，等待spp_handle_init
    moncli_.run();      // 避免spp_handle_init运行很久导致ctrl不断拉起进程的问题    
    CLI_SEND_INFO(&moncli_)->reserve_[0] = 0; //等待spp_handle_init，下次上报时，资源将就绪

    string module_file;
    string module_etc;
    bool module_isGlobal = false;
    //加载用户服务模块
    Module module;

    if (conf.getAndCheckModule(module) == 0)
    {
        module_file = module.bin;
        module_etc = module.etc;
        module_isGlobal = module.isGlobal;
    }
    else
    {
        exit(-1);
    }

    SessionConfig config;
    if (conf.getAndCheckSessionConfig(config) == 0)
    {
        CSessionMgr::Instance()->init(config.etc.c_str(), config.epollTime, TOS_);
    }

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] Load module[%s] etc[%s]\n", pid, module_file.c_str(), module_etc.c_str());

    // 初始化基于命令字和错误码的统计对象
    char stat_mgr_file_name[256];
    snprintf(stat_mgr_file_name, sizeof(stat_mgr_file_name) - 1, SPP_STAT_MGR_FILE, g_spp_groupid);
    ret = CStatMgrInstance::instance()->initStatPool(stat_mgr_file_name);
    if (ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR] init cost stat pool errno = %d\n", ret);
        exit(-1);
    }



    if (0 == load_bench_adapter(module_file.c_str(), module_isGlobal))
    {
        LOG_SCREEN(LOG_FATAL, "call spp_handle_init ...\n");
        int handle_init_ret = 0;

        g_check_point = CoreCheckPoint_HandleInit;
        handle_init_ret = sppdll.spp_handle_init((void*)module_etc.c_str(), this);
        g_check_point = CoreCheckPoint_SppFrame;

        if (handle_init_ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "Worker module %s handle init error, return %d\n",
                       module_file.c_str(), handle_init_ret);
            exit(-1);
        }

        ator_->reg_cb(CB_RECVDATA, ator_recvdata_v2, this);	//数据接收回调注册
    }
    else
    {
        LOG_SCREEN(LOG_ERROR, "Worker load module:%s failed.\n", module_file.c_str());
        exit(-1);
    }

    // 初始化后，即可知道是否异步
    fstat_.init_statobj("worker_type", STAT_TYPE_SET, "微线程[3]/异步[2]/同步[1]类型");
    if (get_mt_flag())
        fstat_.step0("worker_type", 3);
    else if (CSessionMgr::Instance()->_isEnableAsync)
        fstat_.step0("worker_type", 2);
    else
        fstat_.step0("worker_type", 1);

    ret =  CStatMgrInstance::instance()->initFrameStatItem();
    if (ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR] init frame stat errno = %d\n", ret);
        exit(-1);
    }

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] OK!\n", pid);

	_spp_g_get_msg_fun_ptr = get_msg;
	_spp_g_check_stack_fun_ptr = check_stack;

    return 0;
}
int CDefaultWorker::ator_recvdata_v2(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdata v2, flow:%u,blob len:%d\n", flow, blob->len);

    if (likely(blob->len > 0))
    {
        TConnExtInfo* ptr = NULL;

        MONITOR(MONITOR_WORKER_FROM_PROXY);
        blob->len -= sizeof(TConnExtInfo);
        blob->extdata = blob->data + blob->len;
        ptr = (TConnExtInfo*)blob->extdata;

        int64_t recv_ms = int64_t(ptr->recvtime_) * 1000 + ptr->tv_usec / 1000;

        int64_t now = __spp_get_now_ms();

        int64_t time_delay = now - recv_ms;

        worker->fstat_.op(WIDX_MSG_SHM_TIME, time_delay);

        if (_spp_g_exceptionpacketdump)
        {
            PacketData stPackData(blob->data, blob->len, ptr->remoteip_, ptr->localip_, ptr->remoteport_, ptr->localport_, now);
            SavePackage(PACK_TYPE_FRONT_RECV, &stPackData);
        }

        if( worker->msg_timeout_ )
        {        
			shm_delay_stat(time_delay); 
            if ( time_delay > worker->msg_timeout_ )
            {
                MONITOR(MONITOR_WORKER_OVERLOAD_DROP);
                worker->fstat_.op(WIDX_MSG_TIMEOUT, 1);
                worker->flog_.LOG_P_PID(LOG_ERROR, "Flow[%u] Msg Timeout! Delay[%d], Drop!\n"
                    , flow, int(time_delay), blob->len);

                if (UDP_SOCKET == ptr->type_)
                {
                    CTCommu* commu = (CTCommu*)blob->owner;
                    blob_type rspblob;
                    rspblob.len = 0;
                    rspblob.data = NULL;
                    commu->sendto(flow, &rspblob, NULL);
                    worker->flog_.LOG_P_FILE(LOG_DEBUG, "close conn, flow:%u\n", flow);
                }

                return 0;
            }
        }

        worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdone, flow:%u, blob len:%d\n", flow, blob->len);
        worker->fstat_.op(WIDX_SHM_RX_BYTES, blob->len); // 累加接收字节数

        int64_t old = now;
        g_check_point = CoreCheckPoint_HandleProcess;           // 设置待调用插件的CheckPoint
        int ret = sppdll.spp_handle_process(flow, arg1, arg2);
        g_check_point = CoreCheckPoint_SppFrame;                // 恢复CheckPoint，重置为CoreCheckPoint_SppFrame

        if (CSessionMgr::Instance()->_isEnableAsync == false)
        {
            __spp_do_update_tv();
            STEP_REQ_STAT(__spp_get_now_ms()-old);
        }

        if (likely(!ret))
        {
            MONITOR(MONITOR_WORKER_PROC_SUSS);
            return 0;
        }
        else
        {
            MONITOR(MONITOR_WORKER_PROC_FAIL);
            CTCommu* commu = (CTCommu*)blob->owner;
            blob_type rspblob;

            rspblob.len = 0;
            rspblob.data = NULL;

            commu->sendto(flow, &rspblob, NULL);
            worker->flog_.LOG_P_FILE(LOG_DEBUG, "close conn, flow:%u\n", flow);
        }
    }

    return -1;
}

int CDefaultWorker::ator_senddata(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator senddata, flow:%u, blob len:%d\n", flow, blob->len);
    if (blob->len > 0)
    {
        worker->fstat_.op(WIDX_SHM_TX_BYTES, blob->len); // 累加回包字节数
    }

    return 0;
}

int CDefaultWorker::ator_overload(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_ERROR, "worker overload, blob->data: %d\n", (long)blob->data);

    return 0;
}

//add by jeremy
int CDefaultWorker::ator_senderror(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_DEBUG, "ator send error, flow[%u], len[%d]\n", flow, blob->len);

    return 0;
}

