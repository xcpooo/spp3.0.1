/**
 * @brief Nest agent process 
 */
#include<sys/wait.h>

#include <stdio.h>
#include <unistd.h> 
#include <stdint.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sched.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include "nest_clone.h"
#include "nest_proto.h"
#include "agent_msg.h"
#include "agent_conf.h"
#include "nest_log.h"
#include "agent_version.h"
#include "agent_cgroups.h"
#include "agent_net_dispatch.h"
#include "loadxml.h"

using namespace nest;

/* ����ⶨ��, ������ */
int g_spp_groupid;
int g_spp_groups_sum;


// ����cloneʱ�Ĳ������ݽṹ�嶨��
struct TForkArgs
{
    string          path;
    string          bin;
    string          etc;
    string          package;
    string          service;
};


pid_t TMsgProcFork::DoFork(void* args)
{
    size_t stack_size = 4096;
    void *stack = alloca(stack_size);
    pid_t ret = clone(CloneEntry, (char*)stack + stack_size, CLONE_NEWNS | CLONE_PTRACE | SIGCHLD, args);
    if (ret < 0)    
    {        
        NEST_LOG(LOG_ERROR, "clone failed, ret %d(%m)", ret);
        return -1;
    }

    usleep(1);    
    return ret;
}

/**
 * @brief cloneִ�н��̳�ʼ������ں���
 */
int32_t TMsgProcFork::CloneEntry(void* args)
{
    TForkArgs* conf = static_cast<TForkArgs*>(args);
    char output[256];

    // ���pid����Ӧservice��cgroup��tasks
    CAgentCGroups & agent_cgroups = CCloneProxy::Instance()->GetCGroupHandle();
    bool cgroup_ret = agent_cgroups.service_group_add_task(conf->service, (uint32_t)getpid());
    if(!cgroup_ret)
    {
        snprintf(output,sizeof(output), "start proc[%s %s] failed, add cgroup tasks failed, ret=%d",conf->bin.c_str(), conf->etc.c_str(),(int)cgroup_ret);
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        exit(-1);
        return -1;        
    }

    // ����agent exec tool������ҵ�����
    // ../tool/agent_exec_tool bin etc package service
    char *exec_tool = "../tool/agent_exec_tool";
    char *const cmd[] = {exec_tool, 
                         (char*)conf->bin.c_str(), 
                         (char*)conf->etc.c_str(), 
                         (char*)conf->package.c_str(), 
                         (char*)conf->service.c_str(), 
                         (char *)CCloneProxy::_cluster_type.c_str(), 
                         (char*)0};
    int ret = execv(exec_tool, cmd);
    if (ret != 0)
    {
        snprintf(output, sizeof(output), "start proc[%s %s] failed, ret %d", 
                conf->bin.c_str(), conf->etc.c_str(), ret);
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        exit(-2);
        return -2;
    }

    return 0;    
}


// ʵ�ʵ���Ϣ����ӿں���, �̳�ʵ��
int32_t TMsgProcFork::Handle(void* args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CCloneProxy* clone_proxy = CCloneProxy::Instance();

    string head_str, body_str;
    int32_t ret = 0;
    TForkArgs fork_args;
    pid_t child_pid = -1;
    
    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_agent_proc_fork__req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgProcFork detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ��ֵfork����
    fork_args.path = _req_body.path();
    fork_args.bin  = _req_body.bin();
    fork_args.etc  = _req_body.etc();
    fork_args.package = _req_body.package_name();
    fork_args.service = _req_body.service_name();
    
    child_pid = this->DoFork((void*)&fork_args);
    if(child_pid<0)
    {
        NEST_LOG(LOG_ERROR, "TMsgProcFork fork child failed, ret=%d(%m)",(int)child_pid);
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("fork child failed!!");
        goto EXIT_LABEL;           
    }

    // 4. ���óɹ�, ����pid�б���
    _msg_head.set_result(NEST_RESULT_SUCCESS);
    _msg_head.set_err_msg("");

    _rsp_body.set_task_id(_req_body.task_id());
    _rsp_body.set_service_name(_req_body.service_name());
    _rsp_body.set_package_name(_req_body.package_name());
    *_rsp_body.mutable_proc_info()=_req_body.proc_info();
    _rsp_body.set_path(_req_body.path());
    _rsp_body.set_bin(_req_body.bin());
    _rsp_body.set_etc(_req_body.etc());
    _rsp_body.set_proc_pid((uint32_t)child_pid);
    
    NEST_LOG(LOG_DEBUG, "TMsgProcFork fork child rsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
         _rsp_body.ShortDebugString().c_str());

 EXIT_LABEL:

    // 5. ���������Ӧ��
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -1;
    }

    ret = clone_proxy->SendRspMsg(msg->blob_info, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -2;
    }

    return 0;
}



/**
 * @brief ������������
 */
void NestClone::run(int argc, char* argv[])
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
void NestClone::realrun(int argc, char* argv[])
{
    __spp_do_update_tv();

    // 1. ���ó�ʼ������
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    if (initconf(argc, argv) < 0) {
        NEST_LOG(LOG_FATAL, "Nest agent init failed!");
        exit(-1);
    }

    // 3. ���߳���������
    NEST_LOG(LOG_FATAL, "Nest agent clone started!");

    // 4. �����쳣����ê��
    pid_t pid = 0;
    int status = 0;
    int options = 0;
    while (true)
    {
        // �ַ����Ĵ����߼�
        TAgentNetDispatch::Instance()->RunService();

        // ��ʱ�����߼�
        TNestTimerMng::Instance()->RunTimer(__spp_get_now_ms());
        
        // �쳣�߼�����
        if (unlikely(CServerBase::quit()))
        {
            NEST_LOG(LOG_FATAL, "recv quit signal");
            break;
        }

        // ����ӽ����˳�
        pid = 0;
        status = 0;
        options = 0;

        options |= WNOHANG; 
        pid = waitpid(pid,&status,options);
        if(pid > 0)
        {
            NEST_LOG(LOG_ERROR,"work proc %u exit!",(uint32_t)pid);
        }

        // ����ʱ���
        __spp_do_update_tv();
    }

    NEST_LOG(LOG_FATAL, "Nest agent stopped!");
}



/**
 * @brief ���ȴ���ʵ�ʳ�ʼ��
 */
int NestClone::initconf(int argc, char* argv[])
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
    flog.prefix = "nest_clone";
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

    CCloneProxy::_cluster_type = cluster.type;

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
    ret = CCloneProxy::Instance()->Init();
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

/**
 * @brief ���ȴ���ȫ�ֹ�����
 */
CCloneProxy* CCloneProxy::_instance = NULL;
string CCloneProxy::_cluster_type = "spp";

CCloneProxy* CCloneProxy::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new CCloneProxy();
    }

    return _instance;
}

/**
 * @brief ����ȫ�ֵ����ٽӿ�
 */
void CCloneProxy::Destroy()
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
int32_t CCloneProxy::Init()
{
    // 1. ��ʼ��net manager
    this->LoadSetInfo();
    TNestAddrArray svr_list;
    this->ExtractServerList(svr_list);   
    TAgentNetDispatch *net_dispatch = TAgentNetDispatch::Instance();
    net_dispatch->SetMsgDispatch(this);

    // 2, ��ӱ��ؼ�����ַ
    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_CLONE_UNIX_PATH);
    TNestAddress unix_listen;
    unix_listen.SetAddress(&uaddr, true);

    int32_t ret = net_dispatch->AddListen(unix_listen, false);
    
    if (ret < 0)
    {
        NEST_LOG(LOG_FATAL, "Init net manager failed, ret %d!", ret);
        return -1;
    }

    // 2. ������ʱ����
    //TNestTimerMng::Instance()->CreateTimer(CCloneProxy::KeepConnectionTimer, 10, this);

    // 3. ע����Ϣ���
    RegistMsgHandler(NEST_PROTO_TYPE_AGENT, NEST_SCHED_CMD_FORK_PROC, new TMsgProcFork);


    return 0;
}

/**
 * @brief ���ȴ����캯��
 */
CCloneProxy::CCloneProxy()
{
    _set_id     = 0;
}

/**
 * @brief ���ȴ�����������
 */
CCloneProxy::~CCloneProxy()
{

    for (MsgDispMap::iterator it = _msg_map.begin(); it != _msg_map.end(); ++it)
    {
        delete it->second;
    }
    _msg_map.clear();

}

/**
 * @brief ��ȡͨ�õĵ�ַ�б���Ϣ
 */
void CCloneProxy::ExtractServerList(TNestAddrArray& svr_list)
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
int CCloneProxy::dispatch(void* args)
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
    CCloneProxy* agent = CCloneProxy::Instance();
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
void CCloneProxy::RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle)
{
    uint64_t cmd_idx = (((uint64_t)main_cmd) << 32) + sub_cmd;
    this->_msg_map[cmd_idx] = handle;
}

/**
 * @brief ��Ϣ��������ѯ�ӿ�
 */
TNestMsgHandler* CCloneProxy::GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd)
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
void CCloneProxy::LoadSetInfo()
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
 * @brief ����Ӧ������
 */
int32_t CCloneProxy::SendRspMsg(TNestBlob* blob, string& head, string& body)
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
        TNestChannel* channel = TAgentNetDispatch::Instance()->FindChannel(blob->_recv_fd);
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



int main(int argc, char* argv[])
{
    NestClone* agent = new NestClone;
    agent->run(argc, argv);
    delete agent;
    return 0;
}


