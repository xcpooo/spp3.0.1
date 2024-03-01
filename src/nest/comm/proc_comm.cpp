/**
 * @brief nest comm  proxy worker
 */

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "proc_comm.h"
#include "nest_commu.h"
#include "nest_log.h"

using namespace nest;


/**
 * @brief 初始化监听对象
 */
int32_t CProcCtrl::InitListen()
{
    char path[256];
    snprintf(path, sizeof(path), NEST_PROC_UNIX_PATH"_%u", (uint32_t)syscall(__NR_gettid));
    unlink(path);
    
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), "%s", path);

    _listen.SetDispatch(this);
    _listen.SetListenAddr(&uaddr, true);
    int32_t ret = _listen.Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "Init unix datagram listen failed, ret %d", ret);
        return ret;
    }

    return 0;
}

/**
 * @brief 本地报文发送给agent
 */
int32_t CProcCtrl::SendToAgent(void* data, uint32_t len)
{
    struct sockaddr_un uaddr = {0};
    uaddr.sun_family         = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_AGENT_UNIX_PATH);

    TNestAddress address;
    address.SetAddress(&uaddr, true);

    int32_t ret = sendto(_listen.GetSockFd(), data, len, 0, 
                         address.GetAddress(), address.GetAddressLen());
    if (ret < 0)
    {
        NEST_LOG(LOG_DEBUG, "socket[%d] send failed, ret %d (%m)", _listen.GetSockFd(), ret);
    }
    
    return ret;
}

/**
 * @brief 本地报文处理后应答
 */
int32_t CProcCtrl::SendRspMsg(TNestBlob* blob, void* data, uint32_t len)
{
    TNestAddress* address = &blob->_remote_addr;

    int32_t ret = sendto(_listen.GetSockFd(), data, len, 0, 
                          address->GetAddress(), address->GetAddressLen());
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] send failed, remote[%s], ret %d (%m)",
                _listen.GetSockFd(), address->ToString(NULL, 0), ret);
    }
    
    return ret;
}

/**
 * @brief 本地报文处理后应答
 */
int32_t CProcCtrl::SendRspPbMsg(TNestBlob* blob, string& head, string& body)
{
    static char rsp_buff[65535];
    uint32_t buff_len = sizeof(rsp_buff);
    
    if (!blob)
    {
        NEST_LOG(LOG_ERROR, "send msg params invalid, error");
        return -100;
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
        return -200;
    }

    return SendRspMsg(blob, rsp_buff, ret_len);
    
}


/**
 * @brief 本地报文处理后应答
 */
int32_t CProcCtrl::DoRecv(TNestBlob* blob)
{
    // 1. 检查报文合法性
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = blob->_buff;
    pkg_info.buff_len  = blob->_len;

    string err_msg;
    int32_t ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret < 0) 
    {
        NEST_LOG(LOG_ERROR, "pkg parse error, ret %d, msg: %s", ret, err_msg.c_str());    
        return -1;
    }
    
    // 2. 解析报文头信息
    nest_msg_head head;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len))
    {
        NEST_LOG(LOG_ERROR, "pkg head parse failed, log");    
        return -2;
    } 

    // 3. 分发报文处理
    ret = DispatchCtrl(blob, pkg_info, head);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "pkg dispatch failed %d, log", ret);    
        return -3;
    }
    
    return ret;
}


/**
 * @brief 发送定时心跳消息
 */
void CProcCtrl::SendHeartbeat()
{
    static char buff[4096];
    uint32_t len = sizeof(buff);
    nest_msg_head  head;        
    nest_proc_heartbeat_req req_body;

    head.set_msg_type(NEST_PROTO_TYPE_PROC);
    head.set_sub_cmd(NEST_PROC_CMD_HEARTBEAT);
    
    req_body.set_service_name(_service_name);
    req_body.set_package_name(_package_name);    
    nest_proc_base* proc = req_body.mutable_proc();
    proc->set_service_id(_base_conf.service_id());
    proc->set_proc_type(_base_conf.proc_type());
    proc->set_proc_no(_base_conf.proc_no());
    proc->set_proxy_proc_no(_base_conf.proxy_proc_no());
    proc->set_proc_pid(_base_conf.proc_pid());
    proc->set_proxy_ip(_base_conf.proxy_ip());
    proc->set_proxy_port(_base_conf.proxy_port());

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return;
    }

    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = buff;
    rsp_pkg.buff_len    = len;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return;
    }

    this->SendToAgent(buff, ret_len);        
}


/**
 * @brief 发送定时心跳消息
 */
void CProcCtrl::SendLoadReport()
{
    static char buff[4096];
    uint32_t len = sizeof(buff);
    nest_msg_head  head;        
    nest_proc_stat_report_req req_body;
    CMsgStat* stat = TNetCommuMng::Instance()->GetMsgStat();

    head.set_msg_type(NEST_PROTO_TYPE_PROC);
    head.set_sub_cmd(NEST_PROC_CMD_STAT_REPORT);
    
    nest_proc_stat* proc = req_body.mutable_stat();
    proc->set_service_id(_base_conf.service_id());
    proc->set_proc_type(_base_conf.proc_type());
    proc->set_proc_no(_base_conf.proc_no());
    proc->set_proxy_proc_no(_base_conf.proxy_proc_no());
    proc->set_proc_pid(_base_conf.proc_pid());
    proc->set_total_req(stat->GetTotalReq());
    proc->set_total_rsp(stat->GetTotalRsp());
    proc->set_total_cost(stat->GetCostSum());
    proc->set_min_cost(stat->GetMinCost());
    proc->set_max_cost(stat->GetMaxCost());
    proc->set_stat_time(stat->GetStatTime());

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return;
    }

    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = buff;
    rsp_pkg.buff_len    = len;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return;
    }
    
    // 负载每报一次, 清空一次
    stat->Reset((uint64_t)time(NULL));

    this->SendToAgent(buff, ret_len);        
}


/**
 * @brief 分发处理本地示例
 */
int32_t CProcCtrl::DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head)
{
    // 示例处理初始化消息
    // 1. 确认消息类型, 示例, 仅处理初始化消息
    if (head.msg_type() != NEST_PROTO_TYPE_PROC)
    {
        NEST_LOG(LOG_ERROR, "unknown msg type[%u] failed, error", head.msg_type());
        return -1;
    }

    switch (head.sub_cmd())
    {
        case NEST_PROC_CMD_INIT_INFO:
            break;

        default :
            NEST_LOG(LOG_ERROR, "unknown msg subcmd[%u] failed, error", head.sub_cmd());
            return -2;
    }
    
    // 2. 解析包体, 获取setid等信息
    nest_proc_init_req req_body;
    if (!req_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        NEST_LOG(LOG_ERROR, "nest_proc_init_req parse failed, error");
        return -3;
    }
    NEST_LOG(LOG_DEBUG, "proc init msg detail: [%s][%s]", head.ShortDebugString().c_str(),
            req_body.ShortDebugString().c_str());

    return 0;
}


