/**
 * @brief Nest agent process 
 */

#include "nest_agent.h"
#include "nest_proto.h"
#include "agent_process.h"
#include "agent_msg.h"
#include "agent_thread.h"
#include "agent_net_mng.h"
#include "agent_conf.h"
#include "nest_log.h"
#include "agent_version.h"
#include "agent_cgroups.h"
#include "loadxml.h"

using namespace nest;

/* 适配库定义, 无意义 */
int g_spp_groupid;
int g_spp_groups_sum;


/**
 * @brief 进程启动函数
 */
void NestAgent::run(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("\n%s Date:%s, (C) 1998-2014 Tencent Technology.\n", NEST_AGENT_VERSION, __DATE__);
        printf("usage: %s config_file\n\n", argv[0]);
        return;
    }
    
    startup(true);
    realrun(argc, argv);
}

/**
 * @brief 调度代理实际处理函数
 */
void NestAgent::realrun(int argc, char* argv[])
{
    __spp_do_update_tv();

    // 1. 配置初始化处理
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    if (initconf(argc, argv) < 0) {
        NEST_LOG(LOG_FATAL, "Nest agent init failed!");
        exit(-1);
    }

    /*
    取消，使用单独进程，避免运行期间 clone进程bug退出，无法监控和拉起
    // 启动 clone 进程，用来fork worker进程
    if(startCloneProc(argc, argv)!=0)
    {
        NEST_LOG(LOG_FATAL, "Nest agent start clone process failed!");
        exit(-1);        
    }
    */

    // 2. 启动后台线程
    CThreadSysload  sys_thread;
    sys_thread.Start();

    // 3. 主线程启动运行
    NEST_LOG(LOG_FATAL, "Nest agent started!");

    // 4. 进程异常秒起锚点

    while (true)
    {
        // 分发报文处理逻辑
        TAgentNetMng::Instance()->RunService();

        // 定时任务逻辑
        TNestTimerMng::Instance()->RunTimer(__spp_get_now_ms());
        
        // 异常逻辑处理
        if (unlikely(CServerBase::quit()))
        {
            NEST_LOG(LOG_FATAL, "recv quit signal");
            break;
        }

        // 更新时间戳
        __spp_do_update_tv();
    }


    /*
    取消，使用单独进程，避免运行期间 clone进程bug退出，无法监控和拉起
    stopCloneProc();
    */

    NEST_LOG(LOG_FATAL, "Nest agent stopped!");
}

int32_t NestAgent::startCloneProc(int argc, char* argv[])
{
    // 启动agent exec tool，拉起业务进程
    // ../tool/agent_exec_tool bin etc package service
    pid_t pid = fork();

    // fork 失败
    if(pid<0)
    {
        NEST_LOG(LOG_ERROR,"fork clone proc failed, ret=%d (%m)",(int)pid);
        return -1;
    }

    // fork 成功，子进程
    if(pid==0)
    {
        char *exec_agent_clone = "../tool/agent_clone";
        char * cmd[16] = {exec_agent_clone, 
                             0};
        int i=1;
        for(i=1; i<argc && i<15; ++i)
        {
            cmd[i] = argv[i];
        }

        cmd[i] = (char*)0;
        
        int ret = execv(exec_agent_clone, cmd);
        if (ret != 0)
        {
            char output[256]={'\0'};
	    snprintf(output, sizeof(output), "start proc[../tool/agent_clone %s] failed, ret %d", 
                    argv[1], ret);
            NEST_LOG(LOG_ERROR, "%s", output);
            perror(output);
            exit(1);
            return -2;
        }
        // 不会运行到这里
        exit(0);
    }

    if(pid>0)
    {
        // 检查子进程是否运行正常
        sleep(1);
        if ((kill(pid, 0) == -1) && errno == ESRCH)
        {
            NEST_LOG(LOG_ERROR,"clone proc(%d) exit already, bad news", (int32_t)_clone_proc_pid);
            return -1;
        }
    }
    
    _clone_proc_pid = pid;

    NEST_LOG(LOG_ERROR,"clone proc (%d) start ok!",(int32_t)_clone_proc_pid);    
    
    return 0;
}

int32_t NestAgent::stopCloneProc()
{
    if(_clone_proc_pid==-1)
    {
        NEST_LOG(LOG_ERROR,"clone proc stop ok, not started");
        return 0;
    }

    // kill term first
    kill(_clone_proc_pid, SIGTERM);
    
    int32_t kill_cnt = 100; // 100 ms 

    //  等待退出
    for (int32_t i = 0; i < kill_cnt; i++)
    {
        if ((kill(_clone_proc_pid, 0) == -1) && errno == ESRCH)
        {
            NEST_LOG(LOG_ERROR,"clone proc(%d) stop ok", (int32_t)_clone_proc_pid);
            return 0;
        }

        usleep(1000);
    }

    // 没有退出，则使用kill -9 杀死
    kill(_clone_proc_pid, SIGKILL);

    NEST_LOG(LOG_ERROR,"clone proc(%d) stop now, kill -9", (int32_t)_clone_proc_pid);

    return 0;
}


/**
 * @brief 调度代理实际初始化
 */
int NestAgent::initconf(int argc, char* argv[])
{
    CAgentConf conf(new CLoadXML(argv[1]));
    int32_t ret = conf.init();
    if (ret != 0)
    {
        printf("Read conf file failed, error!!\n");
        exit(-1);
    }

    // 1. 日志句柄注册
    Log flog;
    ret = conf.getAndCheckFlog(flog);
    if (ret != 0)
    {
        printf("Read conf file, load flog failed, error!!\n");
        exit(-2);
    }
    flog_.LOG_OPEN(flog.level, flog.type, flog.path.c_str(), flog.prefix.c_str(), 
                   flog.maxfilesize, flog.maxfilenum);

    // 2. 集群相关配置
    ClusterConf cluster;
    ret = conf.getAndCheckCluster(cluster);
    if (ret == ERR_CONF_CHECK_UNPASS)
    {
        printf("Read conf file, load cluster failed, error!!\n");
        exit(-3);
    }

    CAgentProxy::_cluster_type = cluster.type;

    // cgroup config
    ret = conf.getAndCheckCGroupConf(CGroupMng::cgroup_conf);
    if (ret == ERR_CONF_CHECK_UNPASS)
    {
        printf("Read conf file, load cgroup config failed, error!!\n");
        exit(-4);
    }

    // 3. epoll unit设置
    CPollerUnit* poller = TNestNetComm::Instance()->CreatePollUnit();
    TNestNetComm::Instance()->SetPollUnit(poller);

    // 4. agent proxy初始化
    ret = CAgentProxy::Instance()->Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "Nest agent init failed!, ret %d", ret);
        return -1;
    }

    // 5. 设置域socket报文类型的接收队列, 需要root权限
    string max_dgram_qlen = "2000";
    CGroupMng::write("/proc/sys/net/unix/max_dgram_qlen", max_dgram_qlen);
    
    return 0;
}

// 任务超时处理函数
void CTaskCtx::TimerNotify()
{
    NEST_LOG(LOG_TRACE, "Agent ctx timeout, taskid %u!", _taskid);
    
    CAgentProxy* agent = CAgentProxy::Instance();
    if (!agent) 
    {
        NEST_LOG(LOG_ERROR, "Nest agent not init!");
        return;
    }

    agent->DelTaskCtx(_taskid);
}

// 任务超时定时任务启动
void CTaskCtx::EnableTimer(uint32_t wait_time)
{
    CTimerUnit* time_unit = TNestTimerMng::Instance()->GetTimerUnit();
    CTimerList* time_list = NULL;
    if (time_unit) {
        time_list = time_unit->GetTimerList((int)wait_time);
    }

    if (!time_unit || !time_list)
    {
        NEST_LOG(LOG_ERROR, "time unit not init, error!");
        return;
    }
    
    AttachTimer(time_list);
    return;
}


/**
 * @brief 调度代理全局管理句柄
 */
CAgentProxy* CAgentProxy::_instance = NULL;
string CAgentProxy::_cluster_type = "spp";

CAgentProxy* CAgentProxy::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new CAgentProxy();
    }

    return _instance;
}

/**
 * @brief 管理全局的销毁接口
 */
void CAgentProxy::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief 调度代理实际的初始化
 */
int32_t CAgentProxy::Init()
{
    // 1. 初始化net manager
    this->LoadSetInfo();
    TNestAddrArray svr_list;
    this->ExtractServerList(svr_list);    
    int32_t ret = TAgentNetMng::Instance()->InitService(_set_id, svr_list);
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "Init net manager failed, ret %d!", ret);
        return -1;
    }

    // 2. 启动定时任务
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::KeepConnectionTimer, 10, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::HeartbeatTimer, 10*1000, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::LoadReportTimer, 10*1000, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::DcToolTimer, 10*1000, this);

    // 3. 注册消息入口
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_NODE_INIT, new TMsgNodeSet);
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_NODE_TERM, new TMsgNodeDel);
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_ADD_PROC, new TMsgAddProcReq);
    RegistMsgHandler(NEST_PROTO_TYPE_AGENT, NEST_SCHED_CMD_ADD_PROC, new TMsgAddProcRsp);
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_DEL_PROC, new TMsgDelProcReq);
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_SERVICE_INFO, new TMsgSrvInfoReq);
    RegistMsgHandler(NEST_PROTO_TYPE_AGENT, NEST_SCHED_CMD_DEL_PROC, new TMsgDelProcRsp);
    RegistMsgHandler(NEST_PROTO_TYPE_SCHEDULE, NEST_SCHED_CMD_RESTART_PROC, new TMsgRestartProcReq);
    RegistMsgHandler(NEST_PROTO_TYPE_AGENT, NEST_SCHED_CMD_RESTART_PROC, new TMsgRestartProcRsp);
    RegistMsgHandler(NEST_PROTO_TYPE_AGENT, NEST_SCHED_CMD_LOAD_REPORT, new TMsgSysLoadReq);
    RegistMsgHandler(NEST_PROTO_TYPE_PROC, NEST_PROC_CMD_HEARTBEAT, new TMsgProcHeartReq);
    RegistMsgHandler(NEST_PROTO_TYPE_PROC, NEST_PROC_CMD_STAT_REPORT, new TMsgProcLoadReq);

    return 0;
}

/**
 * @brief 调度代理构造函数
 */
CAgentProxy::CAgentProxy()
{
    _set_id     = 0;
}

/**
 * @brief 调度代理析构函数
 */
CAgentProxy::~CAgentProxy()
{
    for (NestTaskMap::iterator it = _task_map.begin(); it != _task_map.end(); ++it)
    {
        delete it->second;
    }
    _task_map.clear();

    for (MsgDispMap::iterator it = _msg_map.begin(); it != _msg_map.end(); ++it)
    {
        delete it->second;
    }
    _msg_map.clear();

}

/**
 * @brief 提取通用的地址列表信息
 */
void CAgentProxy::ExtractServerList(TNestAddrArray& svr_list)
{
    for (uint32_t i = 0; i < _set_servers.size(); i++)
    {
        struct sockaddr_in addr = {0};
        addr.sin_family         = AF_INET;
        addr.sin_addr.s_addr    = inet_addr(_set_servers[i].c_str());
        addr.sin_port           = htons(NEST_SERVER_PORT);
        TNestAddress  nest_addr;
        nest_addr.SetAddress(&addr);
        svr_list.push_back(nest_addr);
    }
}


/**
 * @brief 调度代理实际报文分发
 */
int CAgentProxy::DispatchMsg(void* args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;
    if (!msg || !msg->blob_info || !msg->pkg_info || !msg->nest_head)
    {
        NEST_LOG(LOG_ERROR, "dispatch msg params invalid, error");
        return -100;
    }

    TNestBlob* blob = msg->blob_info;
    nest_msg_head* head = msg->nest_head;
    NEST_LOG(LOG_TRACE, "Agent recv pkg, from %s, msg len: %u", 
                blob->_remote_addr.ToString(NULL, 0), blob->_len);

    // 1. 检查命令字情况
    CAgentProxy* agent = CAgentProxy::Instance();
    uint32_t main_cmd = head->msg_type();
    uint32_t sub_cmd  = head->sub_cmd();
    TNestMsgHandler* handler = agent->GetMsgHandler(main_cmd, sub_cmd);
    if (!handler)
    {
        NEST_LOG(LOG_ERROR, "Agent recv cmd: [%u]-[%u] not registed, from %s, warning!!", 
                main_cmd, sub_cmd,   blob->_remote_addr.ToString(NULL, 0));
        return -200;        
    }

    // 2. 执行该命令
    return handler->Handle(args);
}

/**
 * @brief 消息处理注册接口
 */
void CAgentProxy::RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle)
{
    uint64_t cmd_idx = (((uint64_t)main_cmd) << 32) + sub_cmd;
    this->_msg_map[cmd_idx] = handle;
}

/**
 * @brief 消息处理句柄查询接口
 */
TNestMsgHandler* CAgentProxy::GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd)
{
    uint64_t cmd_idx = (((uint64_t)main_cmd) << 32) + sub_cmd;
    MsgDispMap::iterator it = _msg_map.find(cmd_idx);
    if (it != _msg_map.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief 读取调度的连接信息
 */
void CAgentProxy::LoadSetInfo()
{
    FILE* file = fopen(NEST_AGENT_SET_FILE, "r");
    if (file == NULL) 
    {
        NEST_LOG(LOG_DEBUG, "nest file not exist");
        return;
    }

    /**
     * [SET_ID]         1
     * [SERVER_NUM]     2
     *  0.0.0.0
     *  1.1.1.1
     */
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        char* pos = strstr(line, "[SET_ID]");
        if (pos)
        {
            char setstr[16] = {0};
            sscanf(line, "[SET_ID] %s", setstr);
            _set_id = atoi(setstr);
            continue;
        }

        pos = strstr(line, "[SERVER_NUM]");
        if (pos)
        {
            continue;
        }

        if ((strlen(line) > 7) && !strstr(line, "#"))
        {
            char ipstr[16] = {0};
            sscanf(line, "%s\n", ipstr);
            _set_servers.push_back(ipstr);
            continue;
        }
    }

    fclose(file);
}


/**
 * @brief 读取调度的连接信息
 */
int32_t CAgentProxy::DumpSetInfo()
{
    FILE* file = fopen(NEST_AGENT_SET_FILE, "w+");
    if (file == NULL) 
    {
        NEST_LOG(LOG_DEBUG, "nest file crearte failed");
        return -1;
    }

    /**
     * [SET_ID]         1
     * [SERVER_NUM]     2
     *  0.0.0.0
     *  1.1.1.1
     */
    char line[256];
    snprintf(line, sizeof(line), "[SET_ID] %u\n", _set_id);
    fwrite(line, strlen(line), 1, file);

    snprintf(line, sizeof(line), "[SERVER_NUM] %u\n", (uint32_t)_set_servers.size());
    fwrite(line, strlen(line), 1, file);
    
    for (uint32_t i = 0; i < _set_servers.size(); i++)
    {
        snprintf(line, sizeof(line), "%s\n", _set_servers[i].c_str());
        fwrite(line, strlen(line), 1, file);
    }

    fclose(file);

    return 0;
}

/**
 * @brief 更新调度节点的set信息
 */
int32_t CAgentProxy::UpdateSetInfo(uint32_t setid, TNestAddrArray& servers)
{
    // 1. 检查setid信息
    if ((_set_id != 0) && (_set_id != setid))
    {
        NEST_LOG(LOG_ERROR, "node init already, before setid %u, now setid %u, failed",
            _set_id, setid);
        return -1;
    }

    // 2. 设置保存
    uint32_t prv_set = _set_id;
    std::vector<string> prv_ips = _set_servers;
    _set_id = setid;
    _set_servers.clear();
    for (uint32_t i = 0; i < servers.size(); i++)
    {
        _set_servers.push_back(servers[i].IPString(NULL, 0));    
    }

    // 2. 更新到dump文件
    int32_t ret = this->DumpSetInfo();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "set info update failed, rollback, ret %d", ret);
        _set_id = prv_set;
        _set_servers = prv_ips;
        return -2;
    }

    // 3. 执行更新
    TAgentNetMng::Instance()->UpdateSetInfo(setid, servers);

    return 0;
}


/**
 * @brief 清理set的全局信息
 */
int32_t CAgentProxy::ClearSetInfo()
{
    // 1. 设置保存
    uint32_t prv_set = _set_id;
    std::vector<string> prv_ips = _set_servers;
    _set_id = 0;
    _set_servers.clear();

    // 2. 执行更新
    TNestAddrArray svr_list;
    TAgentNetMng::Instance()->UpdateSetInfo(_set_id, svr_list);

    // 3. 更新到dump文件
    int32_t ret = this->DumpSetInfo();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "set info update failed, rollback, ret %d", ret);
        _set_id = prv_set;
        _set_servers = prv_ips;
        return -1;
    }

    return 0;
}


/**
 * @brief 连接保活的定时函数
 */
int32_t CAgentProxy::KeepConnectionTimer(void* args)
{
    TAgentNetMng::Instance()->CheckKeepAlive();
    return 0;
}


/**
 * @brief 心跳的定时函数
 */
int32_t CAgentProxy::HeartbeatTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent heartbeat time: %llu", __spp_get_now_ms());
    TNestProcMng::Instance()->CheckHeartbeat(__spp_get_now_ms());
    
    return 0;
}

/**
 * @brief 状态上报组合函数
 */
void CAgentProxy::ProcStatSet(nest_proc_base& base, StatUnit& stat, LoadUnit& load, nest_proc_stat* report)
{
    report->set_service_id(base.service_id());
    report->set_proc_type(base.proc_type());
    report->set_proc_no(base.proc_no());
    report->set_proxy_proc_no(base.proxy_proc_no());
    report->set_proc_pid(base.proc_pid());
    
    report->set_cpu_load(load._cpu_used);
    report->set_mem_used(load._mem_used);
    report->set_stat_time(stat._stat_time);
    report->set_total_req(stat._total_req);
    report->set_total_cost(stat._total_cost);
    report->set_total_rsp(stat._total_rsp);
    report->set_min_cost(stat._min_cost);
    report->set_max_cost(stat._max_cost);
}


/**
 * @brief 负载上报的定时函数
 */
int32_t CAgentProxy::LoadReportTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent load report time: %llu", __spp_get_now_ms());
    
    static char msg_buff[8*1024*1024];  // 单机最大5000进程, 心跳包可能比较大
    static uint32_t seq = 0;
    CAgentProxy* agent = CAgentProxy::Instance();
    
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_LOAD_REPORT);
    head.set_set_id(agent->_set_id);
    head.set_sequence(++seq);

    nest_sched_load_report_req req_body;
    req_body.set_node_ip(TAgentNetMng::Instance()->GetListenIp());

    LoadUnit& sys = TNestProcMng::Instance()->GetSysLoad();
    req_body.set_cpu_num(sys._cpu_max);
    req_body.set_cpu_load(sys._cpu_used);
    req_body.set_mem_total(sys._mem_max);
    req_body.set_mem_used(sys._mem_used);

    TProcMap& proc_map = TNestProcMng::Instance()->GetProcMap();
    for (TProcMap::iterator it = proc_map.begin(); it != proc_map.end(); ++it)
    {
        TNestProc* proc = it->second;
        nest_proc_stat* curr_report = req_body.add_stats();
        nest_proc_stat* last_report = req_body.add_last_stats();
        if (!proc || !curr_report || !last_report)
        {
            NEST_LOG(LOG_ERROR, "proc %p, stat %p invalid", proc, curr_report);
            continue;
        }
        
        StatUnit& last_stat = proc->GetLastStat();
        StatUnit& curr_stat = proc->GetCurrStat();
        LoadUnit& load_stat = proc->GetLastLoad();  // CPU目前仅报时刻量
        nest_proc_base& proc_base = proc->GetProcConf();

        // 时间对齐处理, 以当前时间为准, 当前一分钟
        uint32_t now_s = __spp_get_now_s() / 60 * 60;
        if (now_s == curr_stat._stat_time)
        {
            ProcStatSet(proc_base, curr_stat, load_stat, curr_report);
        }
        else
        {
            NEST_LOG(LOG_DEBUG, "now %u, stat %u, not curr !", now_s, curr_stat._stat_time);
        }

        // 时间对齐处理, 以当前时间为准, 上一分钟
        if ((now_s - 60) == last_stat._stat_time)
        {
            ProcStatSet(proc_base, last_stat, load_stat, last_report);
        }
        else
        {
            NEST_LOG(LOG_DEBUG, "now %u, last stat %u, not curr !", now_s, last_stat._stat_time);
        }
    }

    CServiceMng::Instance()->GenPbReport(req_body);
    
    NEST_LOG(LOG_DEBUG, "load report msg detail: [%s][%s]", head.ShortDebugString().c_str(),
            req_body.ShortDebugString().c_str());

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = msg_buff;
    rsp_pkg.buff_len    = sizeof(msg_buff);
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "load msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return -3;
    }

    int32_t ret = TAgentNetMng::Instance()->SendToServer(msg_buff, ret_len);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "send msg failed, ret %d", ret);
        return -4;
    }

    NEST_LOG(LOG_DEBUG, "Agent send load report ok!");

    return 0;
}


/**
 * @brief 检查DC的定时函数
 */
int32_t CAgentProxy::DcToolTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent dc tool time: %llu", __spp_get_now_ms());
    TNestProcMng::Instance()->GetPkgMng().check_run();
    
    return 0;
}



/**
 * @brief 进程应答处理函数
 */
int32_t CAgentProxy::SendRspMsg(TNestBlob* blob, string& head, string& body)
{
    static char rsp_buff[65535];
    uint32_t buff_len = sizeof(rsp_buff);
    
    if (!blob)
    {
        NEST_LOG(LOG_ERROR, "send msg params invalid, error");
        return -1;
    }

    // 1. 调用打包接口, 封装到临时buff
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = rsp_buff;
    rsp_pkg.buff_len    = buff_len;
    rsp_pkg.head_buff   = (void*)head.data();
    rsp_pkg.head_len    = head.size();
    rsp_pkg.body_buff   = (void*)body.data();
    rsp_pkg.body_len    = body.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return -1;
    }

    // 2. 获取报文来源信息, 发送应答
    int32_t ret = 0;
    if (blob->_type != NEST_BLOB_TYPE_STREAM)
    {
        ret = ::sendto(blob->_recv_fd, rsp_buff, ret_len, 0, 
                       blob->_remote_addr.GetAddress(), 
                       blob->_remote_addr.GetAddressLen());
    }
    else
    {
        TNestChannel* channel = TAgentNetMng::Instance()->FindChannel(blob->_recv_fd);
        if (channel && (channel->GetRemoteAddr() == blob->_remote_addr))
        {
            ret = channel->Send(rsp_buff, ret_len);
        }
        else
        {
            NEST_LOG(LOG_ERROR, "fd %u, msg from %s, channel from %s, no match",
                    blob->_recv_fd, blob->_remote_addr.ToString(NULL, 0),
                    channel ? channel->GetRemoteIP() : "PTR NULL");
            ret = -1;
        }
    }
    
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "rsp msg send failed, ret %d, type %u(%m)", ret, blob->_type);
        return -2;
    }

    return 0;
}


/**
 * @brief 进程应答处理函数
 */
int32_t CAgentProxy::SendToAgent(string& head, string& body)
{
    static char rsp_buff[65535];
    uint32_t buff_len = sizeof(rsp_buff);

    // 1. 调用打包接口, 封装到临时buff
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = rsp_buff;
    rsp_pkg.buff_len    = buff_len;
    rsp_pkg.head_buff   = (void*)head.data();
    rsp_pkg.head_len    = head.size();
    rsp_pkg.body_buff   = (void*)body.data();
    rsp_pkg.body_len    = body.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return -1;
    }

    // 2. 获取报文来源信息, 发送应答
    int32_t ret = TAgentNetMng::Instance()->SendtoAgent(rsp_buff, ret_len);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "msg sendto agent failed, ret %d(%m)", ret);
        return -2;
    }

    return 0;
}


/**
 * @brief 添加上下文, 存储任务id信息
 */
void CAgentProxy::AddTaskCtx(uint32_t taskid, CTaskCtx* ctx)
{
    NestTaskMap::iterator it = _task_map.find(taskid);
    if (it != _task_map.end()) {
        delete it->second;
        _task_map.erase(it);
    }
    _task_map[taskid] = ctx;
}

/**
 * @brief 删除上下文
 */
void CAgentProxy::DelTaskCtx(uint32_t taskid)
{
    NestTaskMap::iterator it = _task_map.find(taskid);
    if (it != _task_map.end()) 
    {
        delete it->second;
        _task_map.erase(it);
    }
}

/**
 * @brief 查找上下文信息
 */
CTaskCtx* CAgentProxy::FindTaskCtx(uint32_t taskid)
{
    NestTaskMap::iterator it = _task_map.find(taskid);
    if (it != _task_map.end()) 
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief 全局派发taskid, 本地唯一
 */
uint32_t CAgentProxy::GetTaskSeq()
{
    static uint32_t taskid = 1;
    taskid++;
    if (taskid == 0) {
        taskid++;
    }
    return taskid;
}


