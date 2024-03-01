/**
 * @brief Nest agent msg process 
 */

#include "nest_log.h"
#include "agent_msg.h"
#include "agent_net_mng.h"
#include "agent_thread.h"
#include "nest_agent.h"
#include "agent_process.h"

using namespace nest;


/**
 * @brief ��ȡʵ�ʵĵ���serverip��Ϣ
 */
void TMsgNodeSet::ExtractServers(TNestAddrArray& servers)
{
    for (int32_t i = 0; i < _req_body.servers_size(); i++)
    {
        const string& server = _req_body.servers(i);
        NEST_LOG(LOG_DEBUG, "server index[%d], ip[%s]", i, server.c_str());
    
        in_addr_t s_addr = inet_addr(server.c_str());
        if ((INADDR_NONE == s_addr) || (0 == s_addr)) {
            NEST_LOG(LOG_ERROR, "server index[%d], ip[%s] invalid", i, server.c_str());
            continue;
        }
        
        struct sockaddr_in addr = {0};
        addr.sin_family         = AF_INET;
        addr.sin_addr.s_addr    = s_addr;
        addr.sin_port           = htons(NEST_SERVER_PORT);
        
        TNestAddress  nest_addr;
        nest_addr.SetAddress(&addr);
        servers.push_back(nest_addr);
    }
}

/**
 * @brief ��ȡ��ǰѡ��ĵ���serveripIP
 */
char* TMsgNodeSet::GetCurrServer()
{
    const char* deft_server = "0.0.0.0";
    TNestChannel* keep = TAgentNetMng::Instance()->GetKeepChannel();

    if (keep) {
        return keep->GetRemoteIP();
    } else {
        return (char*)deft_server;
    }
}


/**
 * @brief �ڵ����ýӿڴ�����
 */
int32_t TMsgNodeSet::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    TNestAddrArray servers;
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_node_init_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_TRACE, "TMsgNodeSet detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ȷ���Ƿ�ɸ���set��Ϣ
    if ((agent->GetSetId() != 0) && (agent->GetSetId() != _req_body.set_id()))
    {
        NEST_LOG(LOG_ERROR, "node init already, before setid %u, now setid %u, failed",
            agent->GetSetId(), _req_body.set_id());
        _msg_head.set_result(NEST_ERROR_NODE_SET_NO_MATCH);
        _msg_head.set_err_msg("node init aleady, set id not match!!");
        goto EXIT_LABEL;
    }

    // 4. ��������, �ɹ���dump���ļ�
    this->ExtractServers(servers);
    ret = agent->UpdateSetInfo(_req_body.set_id(), servers);
    if (ret < 0) 
    {
        NEST_LOG(LOG_ERROR, "node init failed, setid %u, ret %d",
            agent->GetSetId(), ret);
        _msg_head.set_result(NEST_ERROR_NODE_INIT_FAILED);
        _msg_head.set_err_msg("node update set info failed!!");
        goto EXIT_LABEL;
    }

    // 5. ���óɹ�����
    _msg_head.set_result(NEST_RESULT_SUCCESS);
    _msg_head.set_err_msg("");
    //rsp_body.set_node_ip();
    _rsp_body.set_set_id(agent->GetSetId());
    _rsp_body.set_task_id(_req_body.task_id());
    _rsp_body.set_server(this->GetCurrServer());

    NEST_LOG(LOG_DEBUG, "TMsgNodeSet rsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
         _rsp_body.ShortDebugString().c_str());

 EXIT_LABEL:

    // 6. ���������Ӧ��
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -1;
    }

    ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -2;
    }

    return 0;

}


/**
 * @brief �ڵ�ɾ���ӿڴ�����
 */
int32_t TMsgNodeDel::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_node_term_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgNodeDel detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ȷ���Ƿ�ɸ���set��Ϣ
    if ((agent->GetSetId() != 0) && (agent->GetSetId() != _req_body.set_id()))
    {
        NEST_LOG(LOG_ERROR, "node set, before setid %u, now setid %u, failed",
            agent->GetSetId(), _req_body.set_id());
        _msg_head.set_result(NEST_ERROR_NODE_SET_NO_MATCH);
        _msg_head.set_err_msg("node term error, set id not match!!");
        goto EXIT_LABEL;
    }

    // 3. ȷ���Ѿ��޽�������
    if (! TNestProcMng::Instance()->GetProcMap().empty())
    {
        NEST_LOG(LOG_ERROR, "node has service run, error",
            agent->GetSetId(), _req_body.set_id());
        _msg_head.set_result(NEST_ERROR_NODE_DEL_HAS_PROC);
        _msg_head.set_err_msg("node term error, more service running!!");
        goto EXIT_LABEL;
    }

    // 4. ��������, �ɹ���dump���ļ�
    ret = agent->ClearSetInfo();
    if (ret < 0) 
    {
        NEST_LOG(LOG_ERROR, "node term failed, setid %u, ret %d",
            agent->GetSetId(), ret);
        _msg_head.set_result(NEST_ERROR_NODE_INIT_FAILED);
        _msg_head.set_err_msg("node clear set info failed!!");
        goto EXIT_LABEL;
    }

    // 5. ���óɹ�����
    _msg_head.set_result(NEST_RESULT_SUCCESS);
    _msg_head.set_err_msg("");


 EXIT_LABEL:

    // 6. ���������Ӧ��
    if (!_msg_head.SerializeToString(&head_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -1;
    }

    ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -2;
    }

    return 0;

}

/**
 * @brief Proxy�Ķ˿��ɱ��ط������, ��������
 */
int32_t TMsgAddProcReq::AllocProxyPort(string& errmsg)
{
    int32_t ret = 0;
    int32_t alloc_count = 0;
    
    for (int32_t i = 0; i < _req_body.proc_conf_size(); i++)
    {
        nest_proc_base* proc_conf = _req_body.mutable_proc_conf(i);
        if (proc_conf->proc_type() != 0)    // ��PROXY ��������
        {
            continue;
        }

        if (this->CheckAllocPort(proc_conf, errmsg) < 0)
        {
            ret = -1;
            goto EXIT_LABEL;
        }
        alloc_count = i;
    }
    
EXIT_LABEL:

    if (ret < 0)    // ʧ�ܰ����������port��Ϣ
    {
        for (int32_t i = 0; i <= alloc_count; i++)
        {
            nest_proc_base* proc_conf = _req_body.mutable_proc_conf(i);
            if (proc_conf->proc_type() != 0)    // ��PROXY ��������
            {
                continue;
            }

            TProcKey key;
            key._service_id     = proc_conf->service_id();
            key._proc_type      = proc_conf->proc_type();
            key._proc_no        = proc_conf->proc_no();
            uint32_t port       = proc_conf->proxy_port();

            NEST_LOG(LOG_DEBUG, "free proxy port %u, proc %u-%u-%u", 
                    port, proc_conf->service_id(),
                    proc_conf->proc_type(), proc_conf->proc_no());

            TNestProcMng::Instance()->FreeUsedPort(key, port);
        }
    }

    return ret;
}

/**
 * @brief ����������ķ����˷�0��port,����ʹ��
 * @return ����port��ռ��, ��proc key����ͬ, ����ʧ�� < 0
 */
int32_t TMsgAddProcReq::CheckAllocPort(nest_proc_base* proc_conf, string& errmsg)
{
    char errinfo[256];

    TProcKey key;
    key._service_id     = proc_conf->service_id();
    key._proc_type      = proc_conf->proc_type();
    key._proc_no        = proc_conf->proc_no();

    string node_ip = TAgentNetMng::Instance()->GetListenIp();
    uint32_t port  = proc_conf->proxy_port();
    if (port == 0)  // ��������δָ��, �������Ȱ�hashѡ��ʹ��, �״δ����ɲ�ָ���������������ָ��
    {
        port = TNestProcMng::Instance()->GetUnusedPort(key);
        if (port != 0)
        {
            proc_conf->set_proxy_port(port);
            
            NEST_LOG(LOG_DEBUG, "proc %u-%u-%u proxy alloc ip[%s] port[%u], ok!", 
                proc_conf->service_id(), proc_conf->proc_type(),
                proc_conf->proc_no(), node_ip.c_str(),port);
        }
        else
        {
            snprintf(errinfo, sizeof(errinfo), "proc %u-%u-%u proxy alloc port failed, failed!!", 
                    proc_conf->service_id(), proc_conf->proc_type(), proc_conf->proc_no());
            errmsg = errinfo;
            NEST_LOG(LOG_ERROR, "%s", errinfo);
            return -1;
        }
    }
    else
    {
        // ����������ָ��port, У�鲢ע��, ʧ���򷵻�
        if (!TNestProcMng::Instance()->RegistProxyPort(key, port))
        {
            snprintf(errinfo, sizeof(errinfo), "proc %u-%u-%u proxy regist port[%u] failed!!", 
                     proc_conf->service_id(), proc_conf->proc_type(), proc_conf->proc_no(), port);
            errmsg = errinfo;
            NEST_LOG(LOG_ERROR, "%s", errinfo);
            return -2;
        }
        
        NEST_LOG(LOG_DEBUG, "proc %u-%u-%u proxy regist ip[%s] port[%u], ok!", 
            proc_conf->service_id(), proc_conf->proc_type(),
            proc_conf->proc_no(), node_ip.c_str(), port);
    }

    // ���ñ��ؼ�����proxy ip, port
    proc_conf->set_proxy_ip(node_ip);

    return 0;
}



/**
 * @brief ��ֹ�ظ������ļ�麯��
 */
int32_t TMsgAddProcReq::CheckProcExist(string& errmsg)
{
    char errinfo[256];
    
    for (int32_t i = 0; i < _req_body.proc_conf_size(); i++)
    {
        const nest_proc_base& proc_conf = _req_body.proc_conf(i);
        
        TProcKey key;
        key._service_id     = proc_conf.service_id();
        key._proc_type      = proc_conf.proc_type();
        key._proc_no        = proc_conf.proc_no();

        TNestProc* proc_item = TNestProcMng::Instance()->FindProc(key);
        if (proc_item != NULL)
        {
            snprintf(errinfo, sizeof(errinfo), "proc %u-%u-%u(%u) exist, repeat create!!", 
                    proc_conf.service_id(), proc_conf.proc_type(), proc_conf.proc_no(),
                    proc_item->GetProcPid());
            errmsg = errinfo;

            NEST_LOG(LOG_ERROR, "%s", errinfo);
            return -1;
        }
    }

    return 0;
}


/**
 * @brief ������ӽӿڴ�����
 */
int32_t TMsgAddProcReq::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;
    bool result = false;
    CTaskCtx*  ctx  = NULL;
    CThreadAddProc* thread = NULL;
    string errmsg;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgAddProcReq msg detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. �������ظ���ӽ���
    if (this->CheckProcExist(errmsg) < 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_req check failed, error");
        _msg_head.set_result(NEST_ERROR_PROC_ALREADY_USED);
        _msg_head.set_err_msg(errmsg);
        goto EXIT_LABEL;
    }

    // TODO ���ض˿ڹ���, ��ʱ�ɱ��ط���
    if (this->AllocProxyPort(errmsg) < 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_req proxy check failed, error");
        _msg_head.set_result(NEST_ERROR_START_PROC_ERROR);
        _msg_head.set_err_msg(errmsg);
        goto EXIT_LABEL;
    }

    // 4. ������ʱsession���̴߳���
    ctx  = new CTaskCtx;
    thread = new CThreadAddProc;
    if (!ctx || !thread)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_req process failed, error");
        _msg_head.set_result(NEST_ERROR_MEM_ALLOC_ERROR);
        _msg_head.set_err_msg("alloc memory failed!!");
        goto EXIT_LABEL;
    }
    ctx->SetTaskId(_req_body.task_id());
    _req_body.set_task_id(agent->GetTaskSeq()); // �滻Ϊ����taskid
    ctx->SetBlobInfo(msg->blob_info);
    ctx->EnableTimer(5*60*1000); // Ĭ��5���ӳ�ʱ
    agent->AddTaskCtx(_req_body.task_id(), ctx);
    
    thread->SetCtx(msg->blob_info, this);
    if (thread->Start() != 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_req start thread failed, error");
        _msg_head.set_result(NEST_ERROR_PROC_THREAD_FAILED);
        _msg_head.set_err_msg("start thread failed!!");
        delete thread; // CTX ��ʱ����
        goto EXIT_LABEL;
    }

    // 5. �ɹ�����
    NEST_LOG(LOG_DEBUG, "nest_sched_add_proc_req start thread ok, task %u, tid: %lu",
            _req_body.task_id(), thread->GetTid());
    result = true;

 EXIT_LABEL:

    if (!result)
    {
        // 6. ʧ������������Ӧ��
        if (!_msg_head.SerializeToString(&head_str))
        {
            NEST_LOG(LOG_ERROR, "pb format string pack failed");
            return -1;
        }

        ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
            return -2;
        }
    }

    return 0;

}


/**
 * @brief ������ӽӿڵ�Ӧ������
 */
void TMsgAddProcRsp::FreeProxyPort()
{
    for (int32_t i = 0; i < _rsp_body.proc_info_size(); i++)
    {
        const nest_proc_base& proc_conf = _rsp_body.proc_info(i);
        if (proc_conf.proc_type() != 0)    // ��PROXY ��������
        {
            continue;
        }
        
        TProcKey key;
        key._service_id     = proc_conf.service_id();
        key._proc_type      = proc_conf.proc_type();
        key._proc_no        = proc_conf.proc_no();
        uint32_t port       = proc_conf.proxy_port();

        NEST_LOG(LOG_DEBUG, "free proxy port %u, proc %u-%u-%u", 
                port, proc_conf.service_id(),
                proc_conf.proc_type(), proc_conf.proc_no());

        TNestProcMng::Instance()->FreeUsedPort(key, port);
    }
}

/**
 * @brief ����ɾ���ӿڵ�Ӧ������
 */
void TMsgAddProcRsp::DeleteProc()
{
    for (int32_t i = 0; i < _rsp_body.proc_info_size(); i++)
    {
        const nest_proc_base& proc_conf = _rsp_body.proc_info(i);
        TNestProc proc;
        proc.SetProcConf(proc_conf);
        TNestProcMng::Instance()->DelProc(&proc);

        NEST_LOG(LOG_TRACE, "Start failed, delete, proc %u-%u-%u(%u)", proc_conf.service_id(),
                proc_conf.proc_type(), proc_conf.proc_no(), proc_conf.proc_pid());
    }
}

/**
 * @brief ������ӽӿڵ�Ӧ������
 */
void TMsgAddProcRsp::UpdateProc()
{
    for (int32_t i = 0; i < _rsp_body.proc_info_size(); i++)
    {
        const nest_proc_base& proc_conf = _rsp_body.proc_info(i);
        TNestProc* proc = new TNestProc;
        if (!proc)
        {
            NEST_LOG(LOG_ERROR, "alloc memory error, log");
            continue;
        }

        proc->SetProcConf(proc_conf);
        proc->SetServiceName(_rsp_body.service_name());
        proc->SetPackageName(_rsp_body.package_name());
        proc->UpdateHeartbeat(__spp_get_now_ms());

        TNestProcMng::Instance()->AddProc(proc);

        NEST_LOG(LOG_TRACE, "start thread ok, proc %u-%u-%u(%u)", proc_conf.service_id(),
                proc_conf.proc_type(), proc_conf.proc_no(), proc_conf.proc_pid());
    }
}


/**
 * @brief ������ӽӿڵ�Ӧ������
 */
int32_t TMsgAddProcRsp::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_rsp_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_rsp parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        //goto EXIT_LABEL;
        return -1; // ����Ŀ�Ĳ���֪
    }
    NEST_LOG(LOG_DEBUG, "TMsgAddProcRsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _rsp_body.ShortDebugString().c_str());

    // 3. �ɹ������ѯctx
    CTaskCtx*  ctx = agent->FindTaskCtx(_rsp_body.task_id());
    if (!ctx)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_add_proc_rsp ctx failed, maybe timeout");
        return -2; 
    }
    uint32_t task_id = _rsp_body.task_id();  // ���汾�ص�taskid   
    _rsp_body.set_task_id(ctx->GetTaskId()); // �滻������taskid

    // 4. �ɹ�������±��ؽ�����Ϣ, ʧ��������˿ڷ�����Ϣ
    if (_msg_head.result() == 0)
    {
        this->UpdateProc();
    }
    else
    {
        this->DeleteProc();
        this->FreeProxyPort();
        _rsp_body.clear_proc_info();
    }

    // 5. Ӧ����, �������Ӧ��
    _msg_head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    if (!_msg_head.SerializeToString(&head_str)
        || (!_rsp_body.SerializeToString(&body_str)))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -3;
    }

    TNestBlob& blob = ctx->GetBlobInfo();
    ret = agent->SendRspMsg(&blob, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -4;
    }

    // 6. ������������Ϣ
    agent->DelTaskCtx(task_id);
    
    return 0;

}


/**
 * @brief ����ɾ���ӿ�ʱ, ������ֹɾ�����������
 */
void TMsgDelProcReq::SetProcDeleteFlag()
{
    // ����������Ŀ, ��ѯ�����Ľ��̵�����, ����5����
    for (int32_t i = 0; i < _req_body.proc_list_size(); i++)
    {
        const nest_proc_base& conf = _req_body.proc_list(i);

        TProcKey key;
        key._service_id     = conf.service_id();
        key._proc_type      = conf.proc_type();
        key._proc_no        = conf.proc_no();    
        TNestProc* proc_item = TNestProcMng::Instance()->FindProc(key);
        if (proc_item != NULL)
        {
            NEST_LOG(LOG_DEBUG, "proc exist, [%u][%u][%u], pid[%u]", conf.service_id(),
                    conf.proc_type(), conf.proc_no(), proc_item->GetProcPid());
            proc_item->UpdateHeartbeat(__spp_get_now_ms() + 5*60*1000);
        }
        else
        {
            NEST_LOG(LOG_ERROR, "proc not exist, [%u][%u][%u]", conf.service_id(),
                    conf.proc_type(), conf.proc_no());
        }
    }
}


/**
 * @brief ����ɾ���ӿڵĴ�����
 */
int32_t TMsgDelProcReq::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;
    bool result = false;
    CTaskCtx*  ctx  = NULL;
    CThreadDelProc* thread = NULL;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_del_proc_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgDelProcReq detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ������ʱsession���̴߳���
    ctx  = new CTaskCtx;
    thread = new CThreadDelProc;
    if (!ctx || !thread)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_del_proc_req process failed, error");
        _msg_head.set_result(NEST_ERROR_MEM_ALLOC_ERROR);
        _msg_head.set_err_msg("alloc memory failed!!");
        goto EXIT_LABEL;
    }
    ctx->SetTaskId(_req_body.task_id());
    _req_body.set_task_id(agent->GetTaskSeq()); // �滻Ϊ����taskid
    ctx->SetBlobInfo(msg->blob_info);
    ctx->EnableTimer(5*60*1000); // Ĭ��5���ӳ�ʱ
    agent->AddTaskCtx(_req_body.task_id(), ctx);
    
    thread->SetCtx(msg->blob_info, this);
    if (thread->Start() != 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_del_proc_req start thread failed, error");
        _msg_head.set_result(NEST_ERROR_PROC_THREAD_FAILED);
        _msg_head.set_err_msg("start thread failed!!");
        delete thread; // CTX ��ʱ����
        goto EXIT_LABEL;
    }

    // 4. ���ý��̼�صı���ʱ�� 5����
    this->SetProcDeleteFlag();

    // 5. �ɹ�����
    NEST_LOG(LOG_DEBUG, "nest_sched_del_proc_req start thread ok, task %u", _req_body.task_id());
    result = true;

 EXIT_LABEL:

    if (!result)
    {
        // 4. ʧ������������Ӧ��
        if (!_msg_head.SerializeToString(&head_str))
        {
            NEST_LOG(LOG_ERROR, "pb format string pack failed");
            return -1;
        }

        ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
            return -2;
        }
    }

    return 0;

}



/**
 * @brief ����ɾ�����´�����
 */
void TMsgDelProcRsp::DeleteProc()
{
    for (int32_t i = 0; i < _rsp_body.proc_list_size(); i++)
    {
        const nest_proc_base& proc_conf = _rsp_body.proc_list(i);

        TProcKey key; 
        key._service_id     = proc_conf.service_id();
        key._proc_type      = proc_conf.proc_type();
        key._proc_no        = proc_conf.proc_no();
        TNestProc* proc = TNestProcMng::Instance()->FindProc(key);
        if (!proc)
        {
            NEST_LOG(LOG_ERROR, "delete thread, proc %u-%u-%u(%u), not found", proc_conf.service_id(),
                    proc_conf.proc_type(), proc_conf.proc_no(), proc_conf.proc_pid());
            TNestProcMng::Instance()->FreeUsedPort(key, 0);  // ������ɾ���߼�, �ݴ�
            continue;
        }

        TNestProcMng::Instance()->FreeUsedPort(key, proc->GetProcConf().proxy_port()); // ����ɾ���߼�       
        TNestProcMng::Instance()->DelProc(proc);
        NEST_LOG(LOG_DEBUG, "delete thread ok, proc %u-%u-%u(%u)", proc_conf.service_id(),
                proc_conf.proc_type(), proc_conf.proc_no(), proc_conf.proc_pid());
    }
}


/**
 * @brief ����ɾ��Ӧ����ӿ�
 */
int32_t TMsgDelProcRsp::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_rsp_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_del_proc_rsp parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        //goto EXIT_LABEL;
        return -1; // ����Ŀ�Ĳ���֪
    }
    NEST_LOG(LOG_DEBUG, "TMsgDelProcRsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _rsp_body.ShortDebugString().c_str());

    // 3. �ɹ������ѯctx
    CTaskCtx*  ctx = agent->FindTaskCtx(_rsp_body.task_id());
    if (!ctx)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_del_proc_rsp ctx failed, maybe timeout");
        return -2; 
    }
    uint32_t task_id = _rsp_body.task_id();  // ���汾�ص�taskid   
    _rsp_body.set_task_id(ctx->GetTaskId()); // �滻������taskid

    // 4. ʧ������������Ӧ��
    _msg_head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    if (!_msg_head.SerializeToString(&head_str)
        || (!_rsp_body.SerializeToString(&body_str)))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -3;
    }

    TNestBlob& blob = ctx->GetBlobInfo();
    ret = agent->SendRspMsg(&blob, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -4;
    }

    // 5. �ɹ�������±��ؽ�����Ϣ, ���taskctx
    agent->DelTaskCtx(task_id);
    if (_msg_head.result() == 0)
    {
        this->DeleteProc();
    }
    return 0;

}


/**
 * @brief ��ȡҵ����Ϣ������
 */
int32_t TMsgSrvInfoReq::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    TNestAddrArray servers;
    string head_str, body_str;
    TProcList proc_list;
    string sname;
    uint32_t proc_cnt;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_service_info_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgSrvInfoReq detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ��ȡҵ�������
    sname = _req_body.service_name();
    TNestProcMng::Instance()->GetServiceProc(sname, proc_list);
    proc_cnt = proc_list.size();
    _rsp_body.set_proc_cnt(proc_cnt);
    _rsp_body.set_task_id(_req_body.task_id());

EXIT_LABEL:
    // 4. ���������Ӧ��
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -1;
    }
 
    ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -2;
    }
 
    return 0;
}


/**
 * @breif ��ҵ����, ��ȡ�������Ľ�����Ϣ
 */
int32_t TMsgRestartProcReq::GetServiceProc(string& errmsg)
{
    TProcList proc_list;
    TNestProcMng::Instance()->GetServiceProc(_req_body.service_name(), proc_list);
    if (proc_list.empty())
    {
        errmsg = "no process to restart!!";
        return -1;
    }

    _req_body.clear_proc_conf();
    for (uint32_t i = 0; i < proc_list.size(); i++)
    {
        nest_proc_base* proc_item = _req_body.add_proc_conf();
        *proc_item = proc_list[i]->GetProcConf();
    }

    return 0;
}

/**
 * @brief ��������pno��Ϣ, ���ú��ʵ�pids��Ϣ
 */
int32_t TMsgRestartProcReq::CheckProcPids(string& errmsg)
{
    char errinfo[256];
    
    for (int32_t i = 0; i < _req_body.proc_conf_size(); i++)
    {
        nest_proc_base* proc_conf = _req_body.mutable_proc_conf(i);
        
        TProcKey key;
        key._service_id     = proc_conf->service_id();
        key._proc_type      = proc_conf->proc_type();
        key._proc_no        = proc_conf->proc_no();

        TNestProc* proc_item = TNestProcMng::Instance()->FindProc(key);
        if (NULL == proc_item)
        {
            snprintf(errinfo, sizeof(errinfo), "proc %u-%u-%u not exist, restart failed!!", 
                    proc_conf->service_id(), proc_conf->proc_type(), proc_conf->proc_no());
            errmsg = errinfo;
            NEST_LOG(LOG_ERROR, "%s", errinfo);
            return -1;
        }
        else
        {
            nest_proc_base& conf_run = proc_item->GetProcConf();
            
            proc_conf->set_proc_pid(conf_run.proc_pid());
            proc_conf->set_proxy_ip(conf_run.proxy_ip());
            proc_conf->set_proxy_port(conf_run.proxy_port());
            proc_conf->set_proxy_proc_no(conf_run.proxy_proc_no());
        }
    }

    return 0;
}

/**
 * @brief ���������������Ϣ�Ƿ����
 */
int32_t TMsgRestartProcReq::CheckRestart(string& errmsg)
{
    int32_t ret = 0;
    
    // 1. setid���ȷ��
    CAgentProxy* agent = CAgentProxy::Instance();
    if (agent->GetSetId() != _msg_head.set_id())
    {
        errmsg = "setid not match, error!!";
        return -1;
    }

    // 2. �����ͼ������
    if (_req_body.proc_all() > 0)
    {
        ret = GetServiceProc(errmsg);
    }
    else
    {
        ret = CheckProcPids(errmsg);
    }

    return ret;
}


/**
 * @brief ������ӽӿڴ�����
 */
int32_t TMsgRestartProcReq::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;
    bool result = false;
    CTaskCtx*  ctx  = NULL;
    CThreadRestartProc* thread = NULL;
    string errmsg;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgRestartProcReq msg detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. �������ظ���ӽ���
    if (this->CheckRestart(errmsg) < 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_req check failed, error");
        _msg_head.set_result(NEST_ERROR_START_PROC_ERROR);
        _msg_head.set_err_msg(errmsg);
        goto EXIT_LABEL;
    }

    // 4. ������ʱsession���̴߳���
    ctx  = new CTaskCtx;
    thread = new CThreadRestartProc;
    if (!ctx || !thread)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_req process failed, error");
        _msg_head.set_result(NEST_ERROR_MEM_ALLOC_ERROR);
        _msg_head.set_err_msg("alloc memory failed!!");
        goto EXIT_LABEL;
    }
    ctx->SetTaskId(_req_body.task_id());
    _req_body.set_task_id(agent->GetTaskSeq()); // �滻Ϊ����taskid
    ctx->SetBlobInfo(msg->blob_info);
    ctx->EnableTimer(5*60*1000); // Ĭ��5���ӳ�ʱ
    agent->AddTaskCtx(_req_body.task_id(), ctx);
    
    thread->SetCtx(msg->blob_info, this);
    if (thread->Start() != 0)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_req start thread failed, error");
        _msg_head.set_result(NEST_ERROR_PROC_THREAD_FAILED);
        _msg_head.set_err_msg("start thread failed!!");
        delete thread; // CTX ��ʱ����
        goto EXIT_LABEL;
    }

    // 5. �ɹ�����
    NEST_LOG(LOG_DEBUG, "nest_sched_restart_proc_req start thread ok, task %u", _req_body.task_id());
    result = true;

 EXIT_LABEL:

    if (!result)
    {
        // 6. ʧ������������Ӧ��
        if (!_msg_head.SerializeToString(&head_str))
        {
            NEST_LOG(LOG_ERROR, "pb format string pack failed");
            return -1;
        }

        ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
            return -2;
        }
    }

    return 0;

}


/**
 * @brief ���������ӿڵ�Ӧ������
 */
int32_t TMsgRestartProcRsp::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_rsp_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_rsp parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        //goto EXIT_LABEL;
        return -1; // ����Ŀ�Ĳ���֪
    }
    NEST_LOG(LOG_DEBUG, "TMsgRestartProcRsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _rsp_body.ShortDebugString().c_str());

    // 3. �ɹ������ѯctx
    CTaskCtx*  ctx = agent->FindTaskCtx(_rsp_body.task_id());
    if (!ctx)
    {
        NEST_LOG(LOG_ERROR, "nest_sched_restart_proc_rsp ctx failed, maybe timeout");
        return -2; 
    }
    uint32_t task_id = _rsp_body.task_id();  // ���汾�ص�taskid   
    _rsp_body.set_task_id(ctx->GetTaskId()); // �滻������taskid

    // 4. ʧ������������Ӧ��
    _msg_head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    if (!_msg_head.SerializeToString(&head_str)
        || (!_rsp_body.SerializeToString(&body_str)))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -3;
    }

    TNestBlob& blob = ctx->GetBlobInfo();
    ret = agent->SendRspMsg(&blob, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -4;
    }

    // 5. �ɹ�������±��ؽ�����Ϣ
    agent->DelTaskCtx(task_id);
    if (_msg_head.result() == 0)
    {
        //this->UpdateProc();  // �ȴ�������Ϣ���½�����Ϣ
    }
    
    return 0;

}


/**
 * @brief ϵͳ�ϱ�����ӿ�
 */
int32_t TMsgSysLoadReq::Handle(void* args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    CAgentProxy* agent = CAgentProxy::Instance();
    string head_str, body_str;
    int32_t ret = 0;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _rsp_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_agent_sysload_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        goto EXIT_LABEL;
    }
    NEST_LOG(LOG_DEBUG, "TMsgSysLoadReq detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ����ϵͳ����
    this->UpdateSysLoad();

    // 4. ���óɹ�, ����pid�б���
    _msg_head.set_result(NEST_RESULT_SUCCESS);
    _msg_head.set_err_msg("");
    this->SetRspPidList();

    NEST_LOG(LOG_DEBUG, "TMsgSysLoadReq rsp detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
         _rsp_body.ShortDebugString().c_str());

 EXIT_LABEL:

    // 5. ���������Ӧ��
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return -1;
    }

    ret = agent->SendRspMsg(msg->blob_info, head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack send failed, ret %d", ret);
        return -2;
    }

    return 0;

}


// ����ϵͳ������Ϣ�����̹���ṹ
int32_t TMsgSysLoadReq::UpdateSysLoad()
{
    uint32_t now = (uint32_t)time(NULL);
    
    // 1. sys load
    LoadUnit sys = {0};
    sys._load_time      = now;
    sys._cpu_max        = _req_body.cpu_total();
    sys._cpu_used       = _req_body.cpu_load();
    sys._mem_max        = _req_body.mem_total();
    sys._mem_used       = _req_body.mem_used();

    TNestProcMng::Instance()->UpdateSysLoad(sys);

    // 2. ������Ϣ����
    for (int32_t i = 0; i < _req_body.stats_size(); i++)
    {
        const nest_proc_stat& proc_stat = _req_body.stats(i);
        uint32_t pid = proc_stat.proc_pid();
        TNestProc* proc = TNestProcMng::Instance()->FindProc(pid);
        if (!proc) {
            NEST_LOG(LOG_ERROR, "pid %u not exist, maybe down", pid);
            continue;
        }
        
        LoadUnit load = {0};
        load._load_time  = now;
        load._cpu_used   = proc_stat.cpu_load();
        load._mem_used   = proc_stat.mem_used();
        proc->SetLoad(load);
    }

    return 0;
}


/**
 * @brief ϵͳ�ϱ�Ӧ��ظ���Ծ��pid�б�
 */
void TMsgSysLoadReq::SetRspPidList()
{
    std::vector<uint32_t> pid_list;
    TNestProcMng::Instance()->GetPidList(pid_list);

    for (uint32_t i = 0; i < pid_list.size(); i++)
    {
        _rsp_body.add_pids(pid_list[i]);
    }
}


/**
 * @brief ���������ϱ�����ӿ�
 */
int32_t TMsgProcHeartReq::Handle(void * args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���
    string head_str, body_str;

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������, ��ȡsetid����Ϣ
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_proc_heartbeat_req parse failed, error");
        _msg_head.set_result(NEST_ERROR_INVALID_BODY);
        _msg_head.set_err_msg("msg body parse failed!!");
        return -1;
    }
    NEST_LOG(LOG_TRACE, "TMsgProcHeartReq detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ��������, ����agent�쳣����, ���³�ʼ������
    this->UpdateHeartbeat();

    // 4. ����Э���ݲ��ذ�

    return 0;

}

// ���½��̵�����ʱ��
void TMsgProcHeartReq::UpdateHeartbeat()
{
    TProcKey key;
    const nest_proc_base& proc_conf = _req_body.proc();
    key._service_id           = proc_conf.service_id();
    key._proc_type            = proc_conf.proc_type();
    key._proc_no              = proc_conf.proc_no();
    TNestProc* proc = TNestProcMng::Instance()->FindProc(key);
    if (!proc)
    {
        NEST_LOG(LOG_ERROR, "recv heartbeat with no proc, maybe agent restart!!");
        this->AddProc();
    }
    else
    {
        if (proc->GetProcPid() != proc_conf.proc_pid())
        {
            NEST_LOG(LOG_ERROR, "recv heartbeat pid change, key[%u]-[%u]-[%u], pid[%u], old[%u] !!",
                key._service_id, key._proc_type, key._proc_no, proc_conf.proc_pid(), proc->GetProcPid());
            TNestProcMng::Instance()->UpdateProcPid(proc, proc_conf.proc_pid());
        }

        // ���ݱ���ʱ��, �������ø�С��ʱ��
        uint64_t now = __spp_get_now_ms();
        uint64_t heart_time = proc->GetHeartbeat();
        if (now > heart_time) 
        {
            proc->UpdateHeartbeat(now);
            NEST_LOG(LOG_TRACE, "proc [%u]-[%u]-[%u] last time[%llu], now[%llu], heart update ok",
                  key._service_id, key._proc_type, key._proc_no, heart_time, now);
        }
        else
        {
            NEST_LOG(LOG_ERROR, "proc [%u]-[%u]-[%u] protect time[%llu], now[%llu], heart not update",
                  key._service_id, key._proc_type, key._proc_no, heart_time, now);
        }

        // �ý��̼���ҵ��cgroups
        CAgentCGroups& cgroups = CAgentProxy::Instance()->GetCGroupHandle();
        cgroups.service_group_add_task(proc->GetServiceName(), proc->GetProcPid());
    }
}


// �����������ֽ��̵��߼�
void TMsgProcHeartReq::AddProc()
{
    TNestProc* proc = new TNestProc;
    if (!proc)
    {
        NEST_LOG(LOG_ERROR, "alloc memory error, log");
        return;
    }
    
    const nest_proc_base& proc_conf = _req_body.proc();
    proc->SetProcConf(proc_conf);
    proc->SetServiceName(_req_body.service_name());
    proc->SetPackageName(_req_body.package_name());
    proc->UpdateHeartbeat(__spp_get_now_ms());
    
    TNestProcMng::Instance()->AddProc(proc);

    // TODO proxy������ռ�˿���Ϣ
    if (proc_conf.proc_type() == 0)
    {
        TProcKey key;
        key._service_id     = proc_conf.service_id();
        key._proc_type      = proc_conf.proc_type();
        key._proc_no        = proc_conf.proc_no();
        uint32_t port       = proc_conf.proxy_port();
        TNestProcMng::Instance()->UpdateUsedPort(key, port);
        
        NEST_LOG(LOG_DEBUG, "port[%u] map update, proc[%u-%u-%u]",
            port, key._service_id, key._proc_type, key._proc_no);
    }
}

/**
 * @brief ���̸����ϱ�����ӿ�
 */
int32_t TMsgProcLoadReq::Handle(void* args)
{
    NestMsgInfo* msg = (NestMsgInfo*)args;      // �ڲ��ӿ�, ��У��Ϸ���

    // 1. �����Ļ�����ʼ��
    NestPkgInfo* pkg_info = msg->pkg_info;    
    nest_msg_head* nest_head = msg->nest_head;    
    _req_body.Clear();
    _msg_head = *nest_head;

    // 2. ��������
    if (!_req_body.ParseFromArray((char*)pkg_info->body_buff, pkg_info->body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_agent_procload_req parse failed, error");
        return -1;
    }
    NEST_LOG(LOG_TRACE, "TMsgProcLoadReq detail: [%s][%s]", _msg_head.ShortDebugString().c_str(),
            _req_body.ShortDebugString().c_str());

    // 3. ����ϵͳ����
    this->UpdateProcLoad();

    return 0;

}


/**
 * @brief ���̸����ϱ����¸�����Ϣ
 */
int32_t TMsgProcLoadReq::UpdateProcLoad()
{
    const nest_proc_stat& proc_stat = _req_body.stat();
    
    uint32_t pid = proc_stat.proc_pid();
    TNestProc* proc = TNestProcMng::Instance()->FindProc(pid);
    if (!proc) {
        NEST_LOG(LOG_ERROR, "pid %u not exist, maybe not init", pid);
        return -1;
    }
    
    StatUnit stat = {0};
    stat._stat_time  = proc_stat.stat_time();
    stat._total_req  = proc_stat.total_req();
    stat._total_cost = proc_stat.total_cost();
    stat._total_rsp  = proc_stat.total_rsp();
    stat._max_cost   = proc_stat.max_cost();
    stat._min_cost   = proc_stat.min_cost();
    
    proc->UpdateStat(stat);

    return 0;
}


