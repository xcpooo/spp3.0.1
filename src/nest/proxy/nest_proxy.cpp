/**
 *@brief Nest proxy process
 */

#include "tinyxmlcomm.h"
#include "proxyconf.h"
#include "misc.h"
#include "comm_def.h"
#include "benchadapter.h"
#include <string.h>
#include "global.h"
#include "monitor.h"
#include "exception.h"
#include "singleton.h"
#include "keygen.h"
#include "misc.h"
#include "notify.h"
#include <libgen.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nest_proto.h"
#include "nest_proxy.h"
#include "nest_commu.h"
#include "nest_log.h"
#include <malloc.h>
#include "timestamp.h"


#define PROXY_STAT_BUF_SIZE 1<<14

#define DEFAULT_ROUTE_NO  1

using namespace spp::comm;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace tbase::notify;
using namespace spp::tconfbase;
using namespace nest;
using namespace spp::exception;

// ??????????so, ????????????
extern struct timeval __spp_g_now_tv;
int g_spp_groupid;
int g_spp_groups_sum;

class mypair
{
public:
    mypair(int a, int b): first(a), second(b) {}
    int first;
    int second;
    bool operator < (const mypair &m)const
    {
        return first < m.first;
    }
};


/**
 * @brief ?????????????????
 */
int32_t CProxyCtrl::DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head)
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
int32_t CProxyCtrl::RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head)
{
    // 1. ??????????
    nest_proc_init_req req_body;
    if (!req_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_proc_init_req parse failed, error");
        return -1;
    }
    NEST_LOG(LOG_TRACE, "msg detail: [%s][%s]", head.ShortDebugString().c_str(),
            req_body.ShortDebugString().c_str());

    // 2. ??????????, ??????????
    _base_conf  = req_body.proc();
    this->SetPackageName(req_body.package_name());
    this->SetServiceName(req_body.service_name());
    
    TCommuSvrDisp& disp = TNetCommuMng::Instance()->GetCommuSvrDisp();
    struct sockaddr_in addr = {0};
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = inet_addr(_base_conf.proxy_ip().c_str());
    addr.sin_port           = htons(_base_conf.proxy_port());
    TNestAddress  nest_addr;
    nest_addr.SetAddress(&addr);
    int32_t ret = disp.StartListen(nest_addr);
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "nest_proc_init_req listen addr[%s:%u] failed, error",
            _base_conf.proxy_ip().c_str(), _base_conf.proxy_port());
        return -2;
    }
    _base_conf.set_proc_pid(getpid());
    
    NEST_LOG(LOG_DEBUG, "proxy listen address[%s:%u] ok, start run!",
        _base_conf.proxy_ip().c_str(), _base_conf.proxy_port());

    // 3. ?л??????
    _server_base->initiated_ = true;

    // 4. switch log to uls
    char frame_log_name[64]   = {0};
    char service_log_name[64] = {0};
    snprintf(frame_log_name, sizeof(frame_log_name) - 1, "nest#%s#frame", req_body.service_name().c_str());
    snprintf(service_log_name, sizeof(service_log_name) - 1, "nest#%s#service", req_body.service_name().c_str());
    ret = _server_base->flog_.switch_net_log(frame_log_name, "", 0, 0, 0);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "switch frame log to net failed, error, ret:%d", ret);
    }

    ret = _server_base->log_.switch_net_log(service_log_name, "", 0, 0, 0);
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
 * @brief ?????????, ???????????????
 */
CNestProxy::CNestProxy(): ator_(NULL), iplimit_(IPLIMIT_DISABLE), initiated_(0)
{
    gettimeofday(&__spp_g_now_tv, NULL);
    this->ctrl_.SetServerBase(this);
}

/**
 * @brief ????????, ???????
 */
CNestProxy::~CNestProxy()
{
    if (ator_ != NULL)
        delete ator_;

    map<int, CTCommu*>::iterator it;
    for (it = ctor_.begin(); it != ctor_.end(); it++)
    {
        if(it->second != NULL)
            delete it->second;
    }
}

/**
 * @brief ?????startup, ??????daemon
 */
void CNestProxy::startup(bool bg_run)
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


/**
 * @brief ???????????, ???????????
 */
int32_t CNestProxy::recv_ctor_rsp()
{
    int32_t processed = 0;    
    map<int32_t, CTCommu*>::iterator it;
    for (it = ctor_.begin(); it != ctor_.end(); it++)
    {
        spp_global::set_cur_group_id(it->first);
        processed += it->second->poll(true); // block
    }

    return processed;
}

/**
 * @brief ?????????????, ???????????
 */
void CNestProxy::handle_before_quit()
{
    NEST_LOG(LOG_FATAL, "recv quit signal\n");

    // 1. ????????????, ????????????
    int32_t count = 20;
    while(count--)
    {
        TNestNetComm::Instance()->RunPollUnit(0);
        this->recv_ctor_rsp();
        usleep(20000);
    }

    // 2. ????????????????
    if (sppdll.spp_handle_fini != NULL) {
        sppdll.spp_handle_fini(NULL, this);
    }
}


/**
 * @brief ?????????????????, ?????????, ????????
 */
void CNestProxy::handle_service()
{
    static int32_t processed = 0; 
    static TCommuSvrDisp& disp = TNetCommuMng::Instance()->GetCommuSvrDisp();

    // 1. ??????п??????????????, ?????????????
    disp.CheckUnregisted(); 
    processed = this->recv_ctor_rsp();

    // 2. ????????????, ???????, ????????? 
    ator_->poll(processed == 0);
}

/**
 * @brief ???????????, ????\????, ???????
 */
void CNestProxy::handle_routine()
{
    static time_t fstattime = 0;
    static time_t montime   = 0;
    static time_t heartbeat = 0;
    static char statbuff[PROXY_STAT_BUF_SIZE];
    char* pstatbuff         = (char*)statbuff;
    int	bufflen             = 0;
    char ctor_stat[1 << 16];
    int ctor_stat_len       = 0;

    __spp_get_now_ms();

    // 1. ????????????????
    if (__spp_g_now_tv.tv_sec != heartbeat)
    {
        heartbeat = __spp_g_now_tv.tv_sec;
        this->ctrl_.SendHeartbeat();
        this->ctrl_.SendLoadReport(); // todo ???1??????????
    }

    // 2. ???????????????
    if (unlikely(__spp_g_now_tv.tv_sec - fstattime > ix_->fstat_inter_time_))
    {
        fstattime = __spp_g_now_tv.tv_sec;
        fstat_.op(PIDX_CUR_CONN, ((CTSockCommu*)ator_)->getconn());
    }

    // 3. ??????????????
    if (unlikely(__spp_g_now_tv.tv_sec - montime > ix_->moni_inter_time_))
    {
        montime = __spp_g_now_tv.tv_sec;
        
        fstat_.result(&pstatbuff, &bufflen, PROXY_STAT_BUF_SIZE);
        //monilog_.LOG_P_NOTIME(LOG_NORMAL, "%s", statbuff);
    
        ctor_stat_len = 0;
        ctor_stat_len += snprintf(ctor_stat, sizeof(ctor_stat), "Connector Stat\n");

        map<int32_t, CTCommu*>::iterator it;
        for (it = ctor_.begin(); it != ctor_.end(); it++)
        {
            it->second->ctrl(0, CT_STAT, ctor_stat, &ctor_stat_len);
        }
    
        NEST_LOG(LOG_NORMAL, "%s\n", ctor_stat);
    }
}


/**
 * @brief ????????????, ???????
 */
void CNestProxy::realrun(int argc, char* argv[])
{
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    initconf(false);
    ix_->fstat_inter_time_ = 5; 

    NEST_LOG(LOG_FATAL, "proxy started!\n");

    while (true)
    {
        // ?????????????, ??п???????
        TNestNetComm::Instance()->RunPollUnit(initiated_ ? 0 : 10);

        // ????????, ???????????????
        if (initiated_) 
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

    NEST_LOG(LOG_FATAL, "proxy stopped!\n");
}


/**
 * @brief ???????????????
 */
bool CNestProxy::init_ctrl_channel()
{
    // poll unit init
    CPollerUnit* poller = TNestNetComm::Instance()->CreatePollUnit();
    TNestNetComm::Instance()->SetPollUnit(poller);
    
    // disp regist
    TCommuSvrDisp& disp = TNetCommuMng::Instance()->GetCommuSvrDisp();
    disp.reg_cb(CB_RECVDATA, ctor_recvdata, this);

    // unix socket init
    if (ctrl_.InitListen() < 0)
    {
        return false;    
    }
    
    return true;
}

/**
 * @brief ??????е????, ???monitor?????, ???commu?????
 */
int CNestProxy::initconf(bool reload)
{
    if ( reload )
    {
        return 0;
    }

    int pid = getpid();

    printf("\nProxy[%5d] init...\n", pid);

    // add nest ctrl init
    if (!init_ctrl_channel())
    {
        NEST_LOG(LOG_FATAL, "proxy ctrol init failed!\n");
        exit(-1);
    }

    CProxyConf conf(new CLoadXML(ix_->argv_[1]));
    int ret = conf.init();
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

    Proxy proxy = conf.getProxy();

    groupid_ = proxy.groupid;
    g_spp_groupid = groupid_;

    //初始化日志

    Log flog;
    ret = conf.getAndCheckFlog(flog);
    if (ret == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    flog_.LOG_OPEN(flog.level, flog.type, flog.path.c_str(), flog.prefix.c_str(), flog.maxfilesize, flog.maxfilenum);

    //初始化业务日志
    Log log;
    ret  = conf.getAndCheckLog(log);
    if (ret == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    log_.LOG_OPEN(log.level, log.type, log.path.c_str(), log.prefix.c_str(), log.maxfilesize, log.maxfilenum);

    //初始化acceptor
    AcceptorSock acceptor;
    ret = conf.getAndCheckAcceptor(acceptor);

    if (ret != 0)
    {
        exit(-1);
    }

    TSockCommuConf socks;
    memset(&socks, 0x0, sizeof(TSockCommuConf));

    socks.udpautoclose_ = acceptor.udpclose;
    socks.maxconn_ = acceptor.maxconn;
    socks.maxpkg_ = acceptor.maxpkg;
    socks.expiretime_ = acceptor.timeout;
    socks.sendcache_limit_ = 0;

    for (size_t i = 0; i < acceptor.entry.size(); ++ i)
    {
        TSockBind binding;
        if (acceptor.entry[i].type == "tcp" || acceptor.entry[i].type == "udp")
        {
            if (acceptor.entry[i].type == "tcp") {
                binding.type_ = SOCK_TYPE_TCP;
                binding.oob_ = acceptor.entry[i].oob;
            }
            else{
                binding.type_ = SOCK_TYPE_UDP;
                binding.oob_ = 0;//udp no oob
            }
            binding.ipport_.ip_ = CMisc::getip(acceptor.entry[i].ip.c_str());
            binding.ipport_.port_ = acceptor.entry[i].port;
            binding.TOS_ = acceptor.entry[i].TOS;
            LOG_SCREEN(LOG_ERROR, "Proxy[%5d] Bind On [%s][%d]...\n",
                pid, acceptor.entry[i].type.c_str(), (int)binding.ipport_.port_);

        }
        else if(acceptor.entry[i].type == "unix")
        {
            binding.type_ = SOCK_TYPE_UNIX;
            binding.isabstract_ = acceptor.entry[i].abstract;
            strncpy(binding.path_, acceptor.entry[i].path.c_str(), 256 - 1);

            LOG_SCREEN(LOG_ERROR, "Proxy[%5d] Bind On [Unix Socket][%s]...\n", pid, binding.path_);
        }
        socks.sockbind_.push_back(binding);
    }

    ator_ = new CTSockCommu;

    int ator_ret = ator_->init(&socks);

    if (ator_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]Proxy acceptor init error, return %d.\n", ator_ret);
        exit(-1);
    }

    ator_->reg_cb(CB_OVERLOAD, ator_overload, this);
    ator_->reg_cb(CB_DISCONNECT, ator_disconnect, this);

    // Add Proxy ctor notify obj
    TNetCommuMng::Instance()->SetNotifyObj((CTSockCommu*)ator_);

    //connector配置

    ConnectShm connector;
    ret = conf.getAndCheckConnector(connector);
    if (ret != 0)
    {
        exit(-1);
    }

    spp_global::mem_full_warning_script = connector.scriptname;

    if (spp_global::mem_full_warning_script != "")
    {
        struct stat fstat;

        if (stat(spp_global::mem_full_warning_script.c_str(), &fstat) == -1 || (fstat.st_mode & S_IFREG) == 0)
        {
            fprintf(stderr, "Attribute scriptname error in config file %s.\n", ix_->argv_[1]);
            spp_global::mem_full_warning_script.clear();
        }
    }


    vector<TShmCommuConf> shms;
    g_spp_groups_sum = connector.entry.size();

    // start: add for nest 20140525
    for (size_t i = 0; i < connector.entry.size(); ++ i)
    {
        int groupid = connector.entry[i].id;
        TNetCommuServer* net_server = new TNetCommuServer(groupid);
        ctor_[groupid] = net_server;
        TNetCommuMng::Instance()->RegistServer(groupid, net_server);
    }
    // end: add for nest 20140525

    //初始化框架统计

    Stat ffstat;
    conf.getAndCheckFStat(ffstat);
    int stat_ret = fstat_.init_statpool(ffstat.file.c_str());
    if (stat_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error, ret:%d, errmsg:%m\n", stat_ret);
        exit(-1);
    }

    fstat_.init_statobj_frame(RX_BYTES, STAT_TYPE_SUM, PIDX_RX_BYTES,
                              "接收 包量/字节数");
    fstat_.init_statobj_frame(TX_BYTES, STAT_TYPE_SUM, PIDX_TX_BYTES,
                              "发送 包量/字节数");
    fstat_.init_statobj_frame(CONN_OVERLOAD, STAT_TYPE_SUM, PIDX_CONN_OVERLOAD,
                              "接入连接失败数");
    fstat_.init_statobj_frame(SHM_ERROR, STAT_TYPE_SUM, PIDX_SHM_ERROR,
                              "共享内存错误次数");
    fstat_.init_statobj_frame(CUR_CONN, STAT_TYPE_SET, PIDX_CUR_CONN,
                              "接入proxy的连接数(tcp和udp协议)");

    //初始化业务统计
    Stat stat;
    conf.getAndCheckStat(stat);

    if (stat_.init_statpool(stat.file.c_str()) != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error.\n");		//modified by jeremy
        exit(-1);
    }

    //初始化监控
    Moni moni;
    ret = conf.getAndCheckMoni(moni);

    if (ret == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    ix_->moni_inter_time_ = moni.intervial;
    monilog_.LOG_OPEN(moni.log.level, moni.log.type, moni.log.path.c_str(),
                      moni.log.prefix.c_str(), moni.log.maxfilesize, moni.log.maxfilenum);

    //加载用户服务模块
    Module module;
    ret = conf.getAndCheckModule(module);
    if (ret != 0)
    {
        exit(-1);
    }
    else
    {
        string module_file;
        string module_etc;
        module_file = module.bin;
        module_etc = module.etc;
        bool module_isGlobal = module.isGlobal;

        LOG_SCREEN(LOG_ERROR, "Proxy[%d] Load module[%s] etc[%s]...\n", pid, module_file.c_str(), module_etc.c_str());

        if (0 == load_bench_adapter(module_file.c_str(), module_isGlobal))
        {
            int handle_init_ret = 0;
            handle_init_ret = sppdll.spp_handle_init((void*)module_etc.c_str(), this);

            if (handle_init_ret != 0)
            {
                LOG_SCREEN(LOG_ERROR, "spp proxy module:%s handle init error, return %d\n",
                           module_file.c_str(), handle_init_ret);
                exit(-1);
            }

            ator_->reg_cb(CB_RECVDATA, ator_recvdata_v2, this);	//数据接收回调注册
        }
        else
        {
            LOG_SCREEN(LOG_ERROR, "Proxy load module:%s failed.\n", module_file.c_str());
            exit(-1);
        }


        string local_handle_name = module.localHandleName;
        if (local_handle_name != "")
        {
            if (sppdll.handle == NULL)
            {
                LOG_SCREEN(LOG_ERROR, "Proxy module:%s handle can not be NULL.\n", module_file.c_str());
                exit(-1);
            }

            local_handle = (spp_handle_process_t)(dlsym(sppdll.handle, local_handle_name.c_str()));

            if (local_handle == NULL)
            {
                LOG_SCREEN(LOG_ERROR, "Proxy module:%s local_handle can not be NULL.\n", module_file.c_str());
                exit(-1);
            }
        }
        else
        {
            local_handle = NULL;
        }
    }

	//加载返回码上报so
	if (!sppdll.spp_handle_report)
	{
	    Result result;
	    ret = conf.getAndCheckResult(result);
		if (ret == 0)
		{
			load_report_adapter(result.bin.c_str());
		}
	}

    //iptable配置
    iplimit_ = IPLIMIT_DISABLE;
    iptable_.clear();
    ator_->reg_cb(CB_CONNECTED, NULL, NULL);

    Iptable ip;
    ret = conf.getAndCheckIptable(ip);
    if (ret == 0)
    {

        string iptable;
        if (ip.whitelist != "")
        {
            iptable = ip.whitelist;
            iplimit_ = IPLIMIT_WHITE_LIST;
        }
        else if (ip.blacklist != "")
        {
            iplimit_ = IPLIMIT_BLACK_LIST;
            iptable = ip.blacklist;
        }


        //read iplist
        if (iplimit_ != IPLIMIT_DISABLE)
        {
            FILE* fp = fopen(iptable.c_str(), "r");

            if (fp)
            {
                char line[32] = {0};
                struct in_addr ipaddr;

                while (fgets(line, 31, fp) != NULL)
                {
                    if (inet_aton(line, &ipaddr))
                        iptable_.insert(ipaddr.s_addr);
                }

                fclose(fp);
            }

            if (iptable_.size())
            {
                ator_->reg_cb(CB_CONNECTED, ator_connected, this);
                NEST_LOG(LOG_NORMAL, "load iptable %d\n", iptable_.size());
            }
            else
                iplimit_ = IPLIMIT_DISABLE;
        }
    }

    LOG_SCREEN(LOG_ERROR, "Proxy[%d] OK!\n", pid);

    return 0;
}
//一些回调函数

int CNestProxy::ator_overload(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestProxy* proxy = (CNestProxy*)arg2;
    proxy->fstat_.op(PIDX_CONN_OVERLOAD, 1);
    NEST_LOG(LOG_ERROR, "proxy overload %d", (long)blob->data);

    return 0;
}

int CNestProxy::ctor_recvdata(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestProxy* proxy = (CNestProxy*)arg2;
    NEST_LOG(LOG_DEBUG, "flow:%u,blob len:%d", flow, blob->len);

    MONITOR(MONITOR_WORKER_TO_PROXY);
    int ret = proxy->ator_->sendto(flow, arg1, NULL);

    if (likely(ret >= 0))
    {
        proxy->fstat_.op(PIDX_TX_BYTES, blob->len); // 累加回包字节数
    }

    return 0;
}

int CNestProxy::ator_recvdata_v2(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestProxy* proxy = (CNestProxy*)arg2;
    NEST_LOG(LOG_DEBUG, "flow:%u,blob len:%d", flow, blob->len);
    int total_len = blob->len;
    int processed_len = 0;
    int proto_len = -1;
    int ret = 0;
    int handle_exception = 0;

    while (blob->len > 0 && (proto_len = sppdll.spp_handle_input(flow, arg1, arg2)) > 0)
    {
        if (proto_len > blob->len)
        {
            NEST_LOG(LOG_ERROR,"spp_handle_input error, flow:%u, blob len:%d, proto_len:%d",
                     flow, blob->len, proto_len);
            processed_len = total_len;
            break;
        }

        ret = 0;

        MONITOR(MONITOR_PROXY_PROC);
        proxy->fstat_.op(PIDX_RX_BYTES, proto_len); // 累加收包字节数
        processed_len += proto_len;

        unsigned route_no;

        if (!sppdll.spp_handle_route)
        {
            route_no = 1;
        }
        else
        {
            // 0x7FFF 为了去掉GROUPID并兼容
            route_no = sppdll.spp_handle_route(flow, arg1, arg2) & 0x7FFF;
        }

        NEST_LOG(LOG_DEBUG, "ator recvdone, flow:%u, blob len:%d, proto_len:%d, route_no:%d",
                 flow, blob->len, proto_len, route_no);
        blob_type sendblob;

        static int datalen = 0;
        static char* data = NULL;
        int need_len = proto_len + sizeof(TConnExtInfo);
        if (datalen < need_len)
        {
            // 每次分配2倍需求空间
            data = (char*)CMisc::realloc_safe(data, 2 * need_len);// 兼容data == NULL
            datalen = 2 * need_len;
        }

        if (data != NULL)
        {
            memcpy(data, blob->data, proto_len);
            memcpy(data + proto_len, blob->extdata, sizeof(TConnExtInfo));
            sendblob.data = data;
            sendblob.len = proto_len + sizeof(TConnExtInfo);
        }
        else
        {
            datalen = 0;
            return proto_len;
        }

        map<int, CTCommu*>::iterator iter;
        if ((iter = proxy->ctor_.find(route_no)) != proxy->ctor_.end())
        {
            MONITOR(MONITOR_PROXY_TO_WORKER);
            ret = iter->second->sendto(flow, &sendblob, NULL);
        }
        else
        {
            NEST_LOG(LOG_ERROR,
                    "group route to %d error, flow:%u, blob len:%d, proto_len:%d\n",
                    route_no, flow, blob->len, proto_len);
        }

        if (unlikely(ret))
        {
            proxy->fstat_.op(PIDX_SHM_ERROR, 1); // 累加共享内存错误次数

            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                        "send to worker error, ret:%d, route_no:%u, flow:%u, blob len:%d, proto_len:%d\n",
                        ret, route_no, flow, blob->len, proto_len);

            //加入业务异常回调控制,当业务认为该异常需要处理的时候，关闭连接
            if(sppdll.spp_handle_exception) {
                handle_exception = sppdll.spp_handle_exception(flow, arg1, arg2);

                if(handle_exception < 0)
                    return handle_exception;
            }
            break;
        }

        blob->data += proto_len;
        blob->len -= proto_len;
    }

    if (proto_len < 0)
    {
        return proto_len;
    }

    return processed_len;
}


int CNestProxy::ator_connected(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CNestProxy* proxy = (CNestProxy*)arg2;
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

    if (proxy->iplimit_ == IPLIMIT_WHITE_LIST)
    {
        if (proxy->iptable_.find(extinfo->remoteip_) == proxy->iptable_.end())
        {
            NEST_LOG(LOG_ERROR,"ip white limited, %s\n",
                     inet_ntoa(*((struct in_addr*)&extinfo->remoteip_)));
            return -1;
        }
    }
    else if (proxy->iplimit_ == IPLIMIT_BLACK_LIST)
    {
        if (proxy->iptable_.find(extinfo->remoteip_) != proxy->iptable_.end())
        {
            NEST_LOG(LOG_ERROR,"ip black limited, %s\n",
                     inet_ntoa(*((struct in_addr*)&extinfo->remoteip_)));
            return -2;
        }
    }

    return 0;
}
// 返回值：0 丢弃数据（默认行为）
//         1 保留数据
int CNestProxy::ator_disconnect(unsigned flow, void* arg1, void* arg2)
{
    int ret = 0;
    blob_type* blob = (blob_type*)arg1;
    CNestProxy* proxy = (CNestProxy*)arg2;

    NEST_LOG(LOG_DEBUG, "flow:%u,blob len:%d\n", flow, blob->len);
    if (sppdll.spp_handle_close == NULL)
    {
        return 0;
    }

    if(blob->len == 0)
    {
        return 0;
    }
    int need_retain = sppdll.spp_handle_close(flow, arg1, arg2);
    if(need_retain == 0)
    {
        return 0;
    }

    proxy->fstat_.op(PIDX_RX_BYTES, blob->len); // 累加收包字节数

    unsigned route_no;

    if (!sppdll.spp_handle_route)
    {
        route_no = 1;
    }
    else
    {
        // 0x7FFF 为了去掉GROUPID并兼容
        route_no = sppdll.spp_handle_route(flow, arg1, arg2) & 0x7FFF;
    }

    NEST_LOG(LOG_DEBUG, "flow:%u, blob len:%d, route_no:%d\n", flow, blob->len, route_no);

    blob_type sendblob;

    static int datalen = 0;
    static char* data = NULL;
    int need_len = blob->len + sizeof(TConnExtInfo);
    if (datalen < need_len)
    {
        //每次分配2倍需求空间
        data = (char*)CMisc::realloc_safe(data, 2 * need_len);// 兼容data == NULL
        datalen = 2 * need_len;
    }

    if (data != NULL)
    {
        memcpy(data, blob->data, blob->len);
        memcpy(data + blob->len, blob->extdata, sizeof(TConnExtInfo));
        sendblob.data = data;
        sendblob.len = blob->len + sizeof(TConnExtInfo);
    }
    else
    {
        datalen = 0;
        return 0;
    }

    map<int, CTCommu*>::iterator iter;
    if ((iter = proxy->ctor_.find(route_no)) != proxy->ctor_.end())
    {
        ret = iter->second->sendto(flow, &sendblob, NULL);
    }
    else
    {
        NEST_LOG(LOG_ERROR, "group route to %d error, flow:%u, blob len:%d\n", route_no, flow, blob->len);
    }

    if (unlikely(ret))
    {
        proxy->fstat_.op(PIDX_SHM_ERROR, 1); // 累加共享内存错误次数
    }

    blob->data += blob->len;
    blob->len -= blob->len;
    return 1;
}

