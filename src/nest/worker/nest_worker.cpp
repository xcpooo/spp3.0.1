#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include "misc.h"
#include "benchadapter.h"
#include "misc.h"
#include "global.h"
#include "singleton.h"
#include "comm_def.h"
#include "tinyxmlcomm.h"
#include "workerconf.h"
#include "keygen.h"
#include "list.h"
#include "notify.h"
#include "../comm/tbase/tsockcommu/tsocket.h"
#include "../comm/exception.h"
#include "../comm/monitor.h"
#include "session_mgr.h"
#include "../async_frame/AsyncFrame.h"

#include "StatMgr.h"
#include "StatMgrInstance.h"
#include "AsyncFrame.h"

#include "nest_proto.h"
#include "nest_worker.h"
#include "nest_commu.h"
#include "nest_log.h"
#include <malloc.h>

#include "SyncFrame.h"


#define WORKER_STAT_BUF_SIZE 1<<14

#define MAX_FILENAME_LEN 255

using namespace spp;
using namespace spp::comm;
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

USING_SPP_STAT_NS;
USING_ASYNCFRAME_NS;


// ??????????so, ????????????
extern CoreCheckPoint g_check_point;
extern struct sigaction g_sa;
extern void sigsegv_handler(int sig);
extern struct timeval __spp_g_now_tv; // session_mgr.cpp
extern int g_spp_L5us;                // NetHandler.cpp 2.10.8 release 2
int g_spp_groupid;
int g_spp_groups_sum;
unsigned int g_worker_type = 0;


#define SPP_GET_NOW_US()   __spp_g_now_tv.tv_usec  // ADD FOR TIME COST CALC 

const std::string get_msg()
{
    if (((CNestWorker*)g_worker)->micro_thread_enable())
    {
        return CSyncFrame::Instance()->GetCurrentMsg();
    }

    return "";
}

bool check_stack(long lESP)
{
    if (((CNestWorker*)g_worker)->micro_thread_enable())
    {
        return CSyncFrame::Instance()->CheckCurrentStack(lESP);
    }

    return true;
}

/**
 * @brief ?????????????????
 */
int32_t CWorkerCtrl::DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head)
{
    // 1. ??????????, ???, ?????????????
    if (head.msg_type() != NEST_PROTO_TYPE_PROC)
    {
        NEST_LOG(LOG_ERROR, "unknown msg type[%u] failed, error", head.msg_type());
        return -1;
    }

    int32_t ret = 0;
    switch (head.sub_cmd())
    {
        case NEST_PROC_CMD_INIT_INFO:
            ret = RecvProcInit(blob, pkg_info, head);
            break;

        default :
            NEST_LOG(LOG_ERROR, "unknown msg subcmd[%u] failed, error", head.sub_cmd());
            return -2;
    }

    ret = ret;
    return 0;
}

/**
 * @brief  ??г???????????
 */
int32_t CWorkerCtrl::RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head)
{
    // ???????????????
    if (_server_base->initiated_)
    {
        NEST_LOG(LOG_ERROR, "nest_proc_init_req already inited, error");
        return -1;
    }

    // 1. ??????????
    nest_proc_init_req req_body;
    if (!req_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_proc_init_req parse failed, error");
        return -1;
    }
    NEST_LOG(LOG_DEBUG, "msg detail: [%s][%s]", head.ShortDebugString().c_str(),
            req_body.ShortDebugString().c_str());

    // 2. ??????????, ??????????
    _base_conf  = req_body.proc();
    this->SetPackageName(req_body.package_name());
    this->SetServiceName(req_body.service_name());

    nest::TNetCoummuConf net_conf;
    net_conf.route_id       = _base_conf.proc_type();
    struct sockaddr_in  ip_addr; 
    ip_addr.sin_family      = AF_INET;
    ip_addr.sin_addr.s_addr = inet_addr(_base_conf.proxy_ip().c_str());
    ip_addr.sin_port        = htons((uint16_t)_base_conf.proxy_port()); 
    net_conf.address.SetAddress(&ip_addr);
    
    TNetCommuClient* client = TNetCommuMng::Instance()->GetCommuClient();
    if (!client || client->init(&net_conf) < 0)
    {
        NEST_LOG(LOG_ERROR, "nest_proc_init_req process failed, error");
        return -1;
    }
    _base_conf.set_proc_pid(getpid());
    

    // 3. ?л??????
    _server_base->initiated_ = true;

    // 4. switch to uls log
    int32_t ret = 0;
    char frame_log_name[64]   = {0};
    char service_log_name[64] = {0};
    snprintf(frame_log_name, sizeof(frame_log_name) - 1, "nest.%s.frame", req_body.service_name().c_str());
    snprintf(service_log_name, sizeof(service_log_name) - 1, "nest.%s.service", req_body.service_name().c_str());
    ret = _server_base->flog_.switch_net_log(frame_log_name, "", 0, 0, net_conf.route_id);     // 框架日志切换到uls
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "switch frame log to net failed, error, ret:%d", ret);
    }

    ret = _server_base->log_.switch_net_log(service_log_name, "", 0, 0, net_conf.route_id);    // 业务日志切换到uls
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "switch service log to net failed, error, ret:%d", ret);
    }

    // 5. ?????????
    nest_proc_init_rsp rsp_body;
    head.set_result(NEST_RESULT_SUCCESS);
    head.set_err_msg("");
    nest_proc_base* conf = rsp_body.mutable_proc();
    *conf = _base_conf;
    rsp_body.set_type(g_worker_type);
    
    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -3;
    }

    return SendRspPbMsg(blob, head_str, body_str);
}



/**
 * @brief  ??????????????????
 */
CNestWorker::CNestWorker(): ator_(NULL), initiated_(0),
    TOS_(-1), notify_fd_(0), mt_flag_(false)
{
    __spp_do_update_tv();
     __spp_get_now_ms();
    ctrl_.SetServerBase(this);
}

CNestWorker::~CNestWorker()
{
    if (ator_ != NULL)
        delete ator_;

}

/**
 * @brief ?????startup, ??????daemon
 */
void CNestWorker::startup(bool bg_run)
{
    struct rlimit rlim;
    if (0 == getrlimit(RLIMIT_NOFILE, &rlim))
    {
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rlim);
        if (rlim.rlim_cur < 100000) { 
            rlim.rlim_cur = 100000;
            rlim.rlim_max = 100000;
            setrlimit(RLIMIT_NOFILE, &rlim);
        }
    }

    mallopt(M_MMAP_THRESHOLD, 1024*1024);
    mallopt(M_TRIM_THRESHOLD, 8*1024*1024);

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if (bg_run)
    {
        signal(SIGINT,  SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        //daemon(1, 1);  // ???????г???fork????, ???????daemon
    }

    CServerBase::flag_ = 0;
    //signal(SIGSEGV, CServerBase::sigsegv_handle);
    signal(SIGUSR1, CServerBase::sigusr1_handle);
    signal(SIGUSR2, CServerBase::sigusr2_handle);
}


// Check micro-thread enable, dynamic or static
bool CNestWorker::micro_thread_enable()
{
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


// Wait micro thread demo run, check swith type
void CNestWorker::micro_thread_switch(bool block)
{
    static CSyncFrame* sync_frame = CSyncFrame::Instance();
    bool flag = sync_frame->GetMtFlag();
    
    // 1. local commu need check notify change
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


/**
 * @brief ?????????????, ???????????
 */
void CNestWorker::handle_before_quit()
{
    int64_t now_ms = 0;

    flog_.LOG_P_PID(LOG_FATAL, "recv quit signal\n");

    __spp_do_update_tv();
    now_ms = __spp_get_now_ms();

    // 1. ????????????, ????????????
    ator_->poll(true);
    int32_t count = 100; // ???400ms, ???????????

    int timeout = 0;
    if (CSessionMgr::Instance()->_isEnableAsync)
    {
        while (count--)
        {
            CSessionMgr::Instance()->run(false);
            if (CAsyncFrame::Instance()->GetMsgCount() <= 0)   // ??????????????, ??????????
            {
                break;
            }
            usleep(4*1000);
        }
    }

    if (mt_flag_)
    {
        while (CSyncFrame::Instance()->GetThreadNum() > 1 && timeout < 1000)
        {
            CSessionMgr::Instance()->check_expired();
            CSyncFrame::Instance()->sleep(10000);
            timeout += 10;
        }
    }

    // 2. ????????????????
    g_check_point = CoreCheckPoint_HandleFini; 
    if (sppdll.spp_handle_fini != NULL)
        sppdll.spp_handle_fini(NULL, this);
    g_check_point = CoreCheckPoint_SppFrame;

    // 3. ????????????
    CStatMgrInstance::destroy();

    __spp_do_update_tv();
    now_ms = __spp_get_now_ms();
    flog_.LOG_P_PID(LOG_FATAL, "exit at %u\n", now_ms);
}


/**
 * @brief ?????????????????, ?????????, ????????
 */
void CNestWorker::handle_service()
{
    // ???????????????????????? ???????
    if (!mt_flag_) 
    {
        // ??????????, ???????????, ?????????
        bool block = (ator_->poll(false) == 0);
        CSessionMgr::Instance()->run(block);
    }
    else
    {
        TNestNetComm::Instance()->RunPollUnit(0); // rcv commu data
        // ??????????, ???????????, ?????????
        bool block = (ator_->poll(false) == 0);
        this->micro_thread_switch(block);         // wait notify
        CSessionMgr::Instance()->check_expired(); // timer
    }
}

/**
 * @brief ???????????, ????\????, ???????
 */
void CNestWorker::handle_routine()
{
    // 0. ????????? handle loop
    if (mt_flag_ && sppdll.spp_handle_loop)
    {
        sppdll.spp_handle_loop(this);   
    }            

    // 1. ????????????????
    static time_t heartbeat = 0;
    time_t now_s = __spp_get_now_s();
    if (now_s != heartbeat)
    {
        heartbeat = now_s;
        this->ctrl_.SendHeartbeat();
        this->ctrl_.SendLoadReport(); 
        TNetCommuMng::Instance()->CheckMsgCtx(__spp_get_now_ms(), 5*60*1000);  // 5??????????
    }

    // 2. 10ms??????????????????????
    static int64_t check_ms = 0;
    int64_t now_ms = __spp_get_now_ms();
    if ((now_ms - check_ms) > 10) 
    {
        check_ms = now_ms;
        TNetCommuMng::Instance()->CheckChannel();
    }
}


/**
 * @brief ???????????????
 */
bool CNestWorker::init_ctrl_channel()
{
    // poll unit init
    bool async_flag = CSessionMgr::Instance()->_isEnableAsync;
    CPollerUnit* poller = CSessionMgr::Instance()->get_pollerunit();
    CSessionMgr::Instance()->_isEnableAsync = async_flag;
    
    TNestNetComm::Instance()->SetPollUnit(poller);
    
    // disp regist recv
    TCommuCltDisp& disp = TNetCommuMng::Instance()->GetCommuCltDisp();
    disp.reg_cb(CB_RECVDATA, ator_recvdata_v2, this);

    // unix socket init
    if (ctrl_.InitListen() < 0)
    {
        return false;    
    }
    
    return true;
}


/**
 * @brief ????????????, ???????
 */
void CNestWorker::realrun(int argc, char* argv[])
{
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    CSessionMgr::Instance()->init_mgr();
    CSessionMgr::Instance()->set_serverbase(this);

    initconf(false);

    flog_.LOG_P_PID(LOG_FATAL, "worker started!\n");
    
    mt_flag_ = this->micro_thread_enable();
    if (mt_flag_) 
    {
        flog_.LOG_P_PID(LOG_FATAL, "micro thread enabled!\n");
    }

    __spp_do_update_tv();

    while (true)
    {

        if (!initiated_)   // ?????????????, ??п???????
        {
            TNestNetComm::Instance()->RunPollUnit(10);
        }
        else  // ????????, ???????????????            
        {
            this->handle_service();
            this->handle_routine();
        }

        // ??????, ??????????
        if (unlikely(CServerBase::quit())) 
        {
            this->handle_before_quit();
            break;
        } 
    }

    flog_.LOG_P_PID(LOG_FATAL, "worker stopped!\n");
}

void CNestWorker::assign_signal(int signo)
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

/**
 * @brief ???ó????, ???nest????????, ???moni??????
 */
int CNestWorker::initconf(bool reload)
{
    if ( reload )
    {
        return 0;
    }
    
    set_serverbase(this);

    int pid = getpid();

    printf("\nWorker[%5d] init...\n", pid);

    // add nest ctrl init
    if (!init_ctrl_channel())
    {
        flog_.LOG_P_PID(LOG_FATAL, "worker ctrol init failed!\n");
        exit(-1);
    }

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

    // start add nest 20140525
    ator_ = new TNetCommuClient;
    if (!ator_)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]Worker acceptor init error, return %d\n", ret);
        exit(-1);
    }
    TNetCommuMng::Instance()->RegistClient((TNetCommuClient*)ator_);
    // end add nest 20140525

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


    CCommu* commu = new CMQCommu(mqkey);
    moncli_.set_commu(commu);
    memset(CLI_SEND_INFO(&moncli_), 0x0, sizeof(TProcInfo));
    CLI_SEND_INFO(&moncli_)->groupid_ = groupid_;

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
    if (micro_thread_enable())
    {
        fstat_.step0("worker_type", 3);
        g_worker_type = 3;
    }
    else if (CSessionMgr::Instance()->_isEnableAsync)
    {
        fstat_.step0("worker_type", 2);
        g_worker_type = 2;
    }
    else
    {
        fstat_.step0("worker_type", 1);
        g_worker_type = 1;
    }

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
int CNestWorker::ator_recvdata_v2(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestWorker* worker = (CNestWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdata v2, flow:%u,blob len:%d\n", flow, blob->len);

    if (likely(blob->len > 0))
    {
        blob->len -= sizeof(TConnExtInfo);
        blob->extdata = blob->data + blob->len;
        TConnExtInfo* ptr = (TConnExtInfo*)blob->extdata;
        //int64_t recv_ms = int64_t(ptr->recvtime_) * 1000 + ptr->tv_usec / 1000;
        int64_t now = __spp_get_now_ms();
        //worker->fstat_.op(WIDX_MSG_SHM_TIME, now - recv_ms);

        MONITOR(MONITOR_WORKER_FROM_PROXY);

        if (_spp_g_exceptionpacketdump)
        {
            PacketData stPackData(blob->data, blob->len, ptr->remoteip_, ptr->localip_, ptr->remoteport_, ptr->localport_, now);
            SavePackage(PACK_TYPE_FRONT_RECV, &stPackData);
        }

        if( worker->msg_timeout_ )
        {
            #if 0	
            if ( now - recv_ms > worker->msg_timeout_ )
            {
                worker->fstat_.op(WIDX_MSG_TIMEOUT, 1);
                worker->flog_.LOG_P_PID(LOG_ERROR, "Flow[%u] Msg Timeout! Delay[%d], Drop!\n"
                    , flow, int(now - recv_ms), blob->len);
                return 0;
            }
            #endif
        }

        worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdone, flow:%u, blob len:%d\n", flow, blob->len);
        worker->fstat_.op(WIDX_SHM_RX_BYTES, blob->len); // 累加接收字节数

        uint64_t start_us = SPP_GET_NOW_US();
        int64_t old = now;
        g_check_point = CoreCheckPoint_HandleProcess;           // 设置待调用插件的CheckPoint
        int ret = sppdll.spp_handle_process(flow, arg1, arg2);
        g_check_point = CoreCheckPoint_SppFrame;                // 恢复CheckPoint，重置为CoreCheckPoint_SppFrame

        if (CSessionMgr::Instance()->_isEnableAsync == false)
        {
            // check update tv called in process
            uint64_t now_us = SPP_GET_NOW_US();
            now = __spp_get_now_ms();            
            if ((now == old) && (now_us == start_us))   // not update
            {
                __spp_do_update_tv();
                now = __spp_get_now_ms();
            }
            STEP_REQ_STAT(now-old);
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

int CNestWorker::ator_senddata(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestWorker* worker = (CNestWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator senddata, flow:%u, blob len:%d\n", flow, blob->len);
    if (blob->len > 0)
    {
        worker->fstat_.op(WIDX_SHM_TX_BYTES, blob->len); // 累加回包字节数
    }

    return 0;
}

int CNestWorker::ator_overload(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestWorker* worker = (CNestWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_ERROR, "worker overload, blob->data: %d\n", (long)blob->data);

    return 0;
}

//add by jeremy
int CNestWorker::ator_senderror(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestWorker* worker = (CNestWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_DEBUG, "ator send error, flow[%u], len[%d]\n", flow, blob->len);

    return 0;
}

