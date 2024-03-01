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

/* ����ⶨ��, ������ */
int g_spp_groupid;
int g_spp_groups_sum;


/**
 * @brief ������������
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
 * @brief ���ȴ���ʵ�ʴ�����
 */
void NestAgent::realrun(int argc, char* argv[])
{
    __spp_do_update_tv();

    // 1. ���ó�ʼ������
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    if (initconf(argc, argv) < 0) {
        NEST_LOG(LOG_FATAL, "Nest agent init failed!");
        exit(-1);
    }

    /*
    ȡ����ʹ�õ������̣����������ڼ� clone����bug�˳����޷���غ�����
    // ���� clone ���̣�����fork worker����
    if(startCloneProc(argc, argv)!=0)
    {
        NEST_LOG(LOG_FATAL, "Nest agent start clone process failed!");
        exit(-1);        
    }
    */

    // 2. ������̨�߳�
    CThreadSysload  sys_thread;
    sys_thread.Start();

    // 3. ���߳���������
    NEST_LOG(LOG_FATAL, "Nest agent started!");

    // 4. �����쳣����ê��

    while (true)
    {
        // �ַ����Ĵ����߼�
        TAgentNetMng::Instance()->RunService();

        // ��ʱ�����߼�
        TNestTimerMng::Instance()->RunTimer(__spp_get_now_ms());
        
        // �쳣�߼�����
        if (unlikely(CServerBase::quit()))
        {
            NEST_LOG(LOG_FATAL, "recv quit signal");
            break;
        }

        // ����ʱ���
        __spp_do_update_tv();
    }


    /*
    ȡ����ʹ�õ������̣����������ڼ� clone����bug�˳����޷���غ�����
    stopCloneProc();
    */

    NEST_LOG(LOG_FATAL, "Nest agent stopped!");
}

int32_t NestAgent::startCloneProc(int argc, char* argv[])
{
    // ����agent exec tool������ҵ�����
    // ../tool/agent_exec_tool bin etc package service
    pid_t pid = fork();

    // fork ʧ��
    if(pid<0)
    {
        NEST_LOG(LOG_ERROR,"fork clone proc failed, ret=%d (%m)",(int)pid);
        return -1;
    }

    // fork �ɹ����ӽ���
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
        // �������е�����
        exit(0);
    }

    if(pid>0)
    {
        // ����ӽ����Ƿ���������
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

    //  �ȴ��˳�
    for (int32_t i = 0; i < kill_cnt; i++)
    {
        if ((kill(_clone_proc_pid, 0) == -1) && errno == ESRCH)
        {
            NEST_LOG(LOG_ERROR,"clone proc(%d) stop ok", (int32_t)_clone_proc_pid);
            return 0;
        }

        usleep(1000);
    }

    // û���˳�����ʹ��kill -9 ɱ��
    kill(_clone_proc_pid, SIGKILL);

    NEST_LOG(LOG_ERROR,"clone proc(%d) stop now, kill -9", (int32_t)_clone_proc_pid);

    return 0;
}


/**
 * @brief ���ȴ���ʵ�ʳ�ʼ��
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

    // 1. ��־���ע��
    Log flog;
    ret = conf.getAndCheckFlog(flog);
    if (ret != 0)
    {
        printf("Read conf file, load flog failed, error!!\n");
        exit(-2);
    }
    flog_.LOG_OPEN(flog.level, flog.type, flog.path.c_str(), flog.prefix.c_str(), 
                   flog.maxfilesize, flog.maxfilenum);

    // 2. ��Ⱥ�������
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

    // 3. epoll unit����
    CPollerUnit* poller = TNestNetComm::Instance()->CreatePollUnit();
    TNestNetComm::Instance()->SetPollUnit(poller);

    // 4. agent proxy��ʼ��
    ret = CAgentProxy::Instance()->Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "Nest agent init failed!, ret %d", ret);
        return -1;
    }

    // 5. ������socket�������͵Ľ��ն���, ��ҪrootȨ��
    string max_dgram_qlen = "2000";
    CGroupMng::write("/proc/sys/net/unix/max_dgram_qlen", max_dgram_qlen);
    
    return 0;
}

// ����ʱ������
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

// ����ʱ��ʱ��������
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
 * @brief ���ȴ���ȫ�ֹ�����
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
 * @brief ����ȫ�ֵ����ٽӿ�
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
 * @brief ���ȴ���ʵ�ʵĳ�ʼ��
 */
int32_t CAgentProxy::Init()
{
    // 1. ��ʼ��net manager
    this->LoadSetInfo();
    TNestAddrArray svr_list;
    this->ExtractServerList(svr_list);    
    int32_t ret = TAgentNetMng::Instance()->InitService(_set_id, svr_list);
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "Init net manager failed, ret %d!", ret);
        return -1;
    }

    // 2. ������ʱ����
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::KeepConnectionTimer, 10, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::HeartbeatTimer, 10*1000, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::LoadReportTimer, 10*1000, this);
    TNestTimerMng::Instance()->CreateTimer(CAgentProxy::DcToolTimer, 10*1000, this);

    // 3. ע����Ϣ���
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
 * @brief ���ȴ����캯��
 */
CAgentProxy::CAgentProxy()
{
    _set_id     = 0;
}

/**
 * @brief ���ȴ�����������
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
 * @brief ��ȡͨ�õĵ�ַ�б���Ϣ
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
 * @brief ���ȴ���ʵ�ʱ��ķַ�
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

    // 1. ������������
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

    // 2. ִ�и�����
    return handler->Handle(args);
}

/**
 * @brief ��Ϣ����ע��ӿ�
 */
void CAgentProxy::RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle)
{
    uint64_t cmd_idx = (((uint64_t)main_cmd) << 32) + sub_cmd;
    this->_msg_map[cmd_idx] = handle;
}

/**
 * @brief ��Ϣ��������ѯ�ӿ�
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
 * @brief ��ȡ���ȵ�������Ϣ
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
 * @brief ��ȡ���ȵ�������Ϣ
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
 * @brief ���µ��Ƚڵ��set��Ϣ
 */
int32_t CAgentProxy::UpdateSetInfo(uint32_t setid, TNestAddrArray& servers)
{
    // 1. ���setid��Ϣ
    if ((_set_id != 0) && (_set_id != setid))
    {
        NEST_LOG(LOG_ERROR, "node init already, before setid %u, now setid %u, failed",
            _set_id, setid);
        return -1;
    }

    // 2. ���ñ���
    uint32_t prv_set = _set_id;
    std::vector<string> prv_ips = _set_servers;
    _set_id = setid;
    _set_servers.clear();
    for (uint32_t i = 0; i < servers.size(); i++)
    {
        _set_servers.push_back(servers[i].IPString(NULL, 0));    
    }

    // 2. ���µ�dump�ļ�
    int32_t ret = this->DumpSetInfo();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "set info update failed, rollback, ret %d", ret);
        _set_id = prv_set;
        _set_servers = prv_ips;
        return -2;
    }

    // 3. ִ�и���
    TAgentNetMng::Instance()->UpdateSetInfo(setid, servers);

    return 0;
}


/**
 * @brief ����set��ȫ����Ϣ
 */
int32_t CAgentProxy::ClearSetInfo()
{
    // 1. ���ñ���
    uint32_t prv_set = _set_id;
    std::vector<string> prv_ips = _set_servers;
    _set_id = 0;
    _set_servers.clear();

    // 2. ִ�и���
    TNestAddrArray svr_list;
    TAgentNetMng::Instance()->UpdateSetInfo(_set_id, svr_list);

    // 3. ���µ�dump�ļ�
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
 * @brief ���ӱ���Ķ�ʱ����
 */
int32_t CAgentProxy::KeepConnectionTimer(void* args)
{
    TAgentNetMng::Instance()->CheckKeepAlive();
    return 0;
}


/**
 * @brief �����Ķ�ʱ����
 */
int32_t CAgentProxy::HeartbeatTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent heartbeat time: %llu", __spp_get_now_ms());
    TNestProcMng::Instance()->CheckHeartbeat(__spp_get_now_ms());
    
    return 0;
}

/**
 * @brief ״̬�ϱ���Ϻ���
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
 * @brief �����ϱ��Ķ�ʱ����
 */
int32_t CAgentProxy::LoadReportTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent load report time: %llu", __spp_get_now_ms());
    
    static char msg_buff[8*1024*1024];  // �������5000����, ���������ܱȽϴ�
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
        LoadUnit& load_stat = proc->GetLastLoad();  // CPUĿǰ����ʱ����
        nest_proc_base& proc_base = proc->GetProcConf();

        // ʱ����봦��, �Ե�ǰʱ��Ϊ׼, ��ǰһ����
        uint32_t now_s = __spp_get_now_s() / 60 * 60;
        if (now_s == curr_stat._stat_time)
        {
            ProcStatSet(proc_base, curr_stat, load_stat, curr_report);
        }
        else
        {
            NEST_LOG(LOG_DEBUG, "now %u, stat %u, not curr !", now_s, curr_stat._stat_time);
        }

        // ʱ����봦��, �Ե�ǰʱ��Ϊ׼, ��һ����
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
 * @brief ���DC�Ķ�ʱ����
 */
int32_t CAgentProxy::DcToolTimer(void* args)
{
    NEST_LOG(LOG_DEBUG, "Agent dc tool time: %llu", __spp_get_now_ms());
    TNestProcMng::Instance()->GetPkgMng().check_run();
    
    return 0;
}



/**
 * @brief ����Ӧ������
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

    // 1. ���ô���ӿ�, ��װ����ʱbuff
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

    // 2. ��ȡ������Դ��Ϣ, ����Ӧ��
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
 * @brief ����Ӧ������
 */
int32_t CAgentProxy::SendToAgent(string& head, string& body)
{
    static char rsp_buff[65535];
    uint32_t buff_len = sizeof(rsp_buff);

    // 1. ���ô���ӿ�, ��װ����ʱbuff
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

    // 2. ��ȡ������Դ��Ϣ, ����Ӧ��
    int32_t ret = TAgentNetMng::Instance()->SendtoAgent(rsp_buff, ret_len);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "msg sendto agent failed, ret %d(%m)", ret);
        return -2;
    }

    return 0;
}


/**
 * @brief ���������, �洢����id��Ϣ
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
 * @brief ɾ��������
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
 * @brief ������������Ϣ
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
 * @brief ȫ���ɷ�taskid, ����Ψһ
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


