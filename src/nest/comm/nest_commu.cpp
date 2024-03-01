/**
 * @brief net commu for proxy worker
 */

#include <errno.h>
#include <sys/ioctl.h>
#include "global.h"
#include "tsockcommu.h"
#include "nest_commu.h"
#include "timestamp.h"
#include "nest_log.h"

using namespace nest;


/**
 * @brief 内部消息交互的协议头转换函数
 */
void TCommuDisp::ntoh_msg_head(TNetCommuHead* head_info)
{
    head_info->magic_num     = ntohl(head_info->magic_num);
    head_info->msg_len       = ntohl(head_info->msg_len);
    head_info->head_len      = ntohl(head_info->head_len);
    head_info->msg_type      = ntohl(head_info->msg_type);
    head_info->route_id      = ntohl(head_info->route_id);
    head_info->flow          = ntohl(head_info->flow);
    head_info->crc           = ntohl(head_info->crc);
    head_info->from_ip       = ntohl(head_info->from_ip);
    head_info->from_port     = ntohl(head_info->from_port);
    head_info->rcv_sec       = ntohl(head_info->rcv_sec);
    head_info->rcv_usec      = ntohl(head_info->rcv_usec);
    head_info->time_cost     = ntohl(head_info->time_cost);
}

/**
 * @brief 内部消息交互的协议头转换函数
 */
void TCommuDisp::hton_msg_head(TNetCommuHead* head_info)
{
    head_info->magic_num     = htonl(head_info->magic_num);
    head_info->msg_len       = htonl(head_info->msg_len);
    head_info->head_len      = htonl(head_info->head_len);
    head_info->msg_type      = htonl(head_info->msg_type);
    head_info->route_id      = htonl(head_info->route_id);
    head_info->flow          = htonl(head_info->flow);
    head_info->crc           = htonl(head_info->crc);
    head_info->from_ip       = htonl(head_info->from_ip);
    head_info->from_port     = htonl(head_info->from_port);
    head_info->rcv_sec       = htonl(head_info->rcv_sec);
    head_info->rcv_usec      = htonl(head_info->rcv_usec);
    head_info->time_cost     = htonl(head_info->time_cost);
}

/**
 * @brief 如果流ID一个请求, 多个应答
 */
void TCommuDisp::default_msg_head(TNetCommuHead* head_info, uint32_t flow, uint32_t group_id)
{
    memset(head_info, 0, sizeof(*head_info));    
    head_info->magic_num = NEST_MAGIC_NUM;
    head_info->msg_len   = 0;        // 需要重赋值设置的
    head_info->flow      = flow; 
    head_info->head_len  = sizeof(*head_info);
    head_info->msg_type  = 0;
    head_info->route_id  = group_id;
    head_info->rcv_sec   = 0;
    head_info->rcv_usec  = 0;
    head_info->crc       = 0;
}

/**
 * @brief  检查CRC值正确性
 * @return true:CRC值正确  false:CRC错误
 */
bool TCommuDisp::check_crc(TNetCommuHead* head_info)
{
    uint32_t *head = (uint32_t *)head_info;
    uint32_t  crc  = *head;

    for (uint32_t i = 1; i < sizeof(TNetCommuHead)/sizeof(uint32_t); i++)
    {
        crc ^= *(head + i);
    }

    if (crc)   // 如果非零，则XOR错误
    {
        return false;
    }

    return true;
}

/**
 * @brief 生成msg_head的CRC值
 * @return 返回CRC值
 */
uint32_t TCommuDisp::generate_crc(TNetCommuHead* head_info)
{
    uint32_t *head = (uint32_t *)head_info;
    uint32_t  crc  = *head;

    head_info->crc = 0;  // xor值初始化为零
    for (uint32_t i = 1; i < sizeof(TNetCommuHead)/sizeof(uint32_t); i++)
    {
        crc ^= *(head + i);
    }

    return crc;
}

/** 
 * @brief 检查消息的格式, 返回报文长度, 头长度
 * @param blob 消息块
 * @param head_len  头长度, 返回>0时有效
 * @return 消息长度, 0 -待继续接收; >0 消息长度; <0无效报文
 */
int32_t TCommuDisp::CheckMsg(void* pos, uint32_t left, TNetCommuHead& head_info)
{
    if (left < sizeof(TNetCommuHead))
    {
        NEST_LOG(LOG_DEBUG, "msg head not ok, wait!!");    
        return 0;
    }

    // 消息结构 TNetCommuHead, 与消息字节流要区分开, 后续可调整这里
    TNetCommuHead* head_ptr = (TNetCommuHead*)pos;
    head_info = *head_ptr;
    TCommuDisp::ntoh_msg_head(&head_info);

    // check magic num
    if (head_info.magic_num != NEST_MAGIC_NUM)
    {
        NEST_LOG(LOG_ERROR, "magic num not match %x", head_info.magic_num);  
        return -1;
    }        

    // check xor val
    if (!TCommuDisp::check_crc(&head_info))
    {
        NEST_LOG(LOG_ERROR, "msg head crc check failed");  
        return -11;
    }
    
    uint32_t msg_len = head_info.msg_len;
    if (left < msg_len)
    {
        NEST_LOG(LOG_DEBUG, "msg len %u not ok, wait!!", msg_len);  
        return 0;
    }

    // 获取block, 区分控制消息与业务消息
    if (head_info.head_len > msg_len)
    {
        NEST_LOG(LOG_ERROR, "msg len %u, head len %u, not match!!", msg_len, head_info.head_len);  
        return -2;
    }

    return (int32_t)msg_len;
}


/**
 * @brief 业务消息处理函数, 虚接口函数
 */
void TCommuDisp::HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body)
{
    NEST_LOG(LOG_TRACE, "recv service msg, fd %u, type %u, group %u", blob->_recv_fd, 
            head_info.msg_type, head_info.route_id); 

    if (func_list_[CB_RECVDATA]) 
    {
        func_list_[CB_RECVDATA](head_info.flow, &body, func_args_[CB_RECVDATA]);
    } 
    else 
    {
        NEST_LOG(LOG_ERROR, "no recv func regist, error"); 
    }

    return;
}

/**
 * @brief  适配原有的处理流程, 先接收, 后处理
 */
int32_t TCommuDisp::DoRecvCache(TNestBlob* blob, TNestChannel* channel, bool block)
{
    int32_t process_len = 0;
    char* pos           = (char*)blob->_buff;
    uint32_t left       = blob->_len;

    do
    {
        TNetCommuHead head;
        int32_t msg_len = this->CheckMsg(pos, left, head);
        if (msg_len < 0)        // 异常处理
        {
            return msg_len;
        }        
        else if (msg_len == 0)  // 无完整报文
        {
            return process_len;
        }
        
        blob_type blob_body;
        blob_body.data  = pos     + head.head_len;
        blob_body.len   = msg_len - head.head_len;
        blob_body.owner = NULL; // SERVER 不需要, CLIENT 需要, 在HandleService中更新
        if (head.msg_type != 0)
        {
            NEST_LOG(LOG_TRACE, "recv contrl msg, type: %u", head.msg_type); 
            this->HandleControl(blob, head, blob_body);
        }
        else
        {
            this->HandleService(blob, head, blob_body);
            this->DoRecvFinished(blob, channel, head);
        }

        pos         += msg_len;
        left        -= msg_len;
        process_len += msg_len;

    }while (block);

    return process_len;

}

// 虚拟析构函数
TCommuSvrDisp::~TCommuSvrDisp()
{
    TNestChannelMap::iterator it;
    for (it = _channel_map.begin(); it != _channel_map.end(); ++it)
    {
        this->DelChannel(it->second);
    }
    _channel_map.clear();
    _un_rigsted.clear();    
}


// 处理监听新连接接口, 返回0成功, 其他失败
int32_t TCommuSvrDisp::DoAccept(int32_t cltfd, TNestAddress* addr)
{
    TNestChannel* inner_obj = new TNestChannel;
    if (!inner_obj)
    {
        NEST_LOG(LOG_ERROR, "Tcp channel alloc failed, fd %d", cltfd);    
        close(cltfd);
        return -1;
    }
    inner_obj->SetSockFd(cltfd);
    inner_obj->SetRemoteAddr(addr);
    inner_obj->SetConnState(CONNECTED);
    inner_obj->SetDispatch(this);

    // 监听事件注册
    inner_obj->EnableInput();
    inner_obj->DisableOutput();
    if (inner_obj->AttachPoller(TNestNetComm::Instance()->GetPollUnit()) <0)
    {
        NEST_LOG(LOG_ERROR, "Tcp channel event attach failed, fd %d", cltfd);    
        close(cltfd);
        delete inner_obj;
        return -2;
    }
    
    // 管理消息通道
    this->AddChannel(inner_obj);

    return 0;

}


// 异常处理接口, 可直接回收channel指针
int32_t TCommuSvrDisp::DoError(TNestChannel* channel) 
{
    if (!channel) {
        NEST_LOG(LOG_ERROR, "channel null, error!!");    
        return -1;
    }

    this->DelChannel(channel);

    return 0;
};

/**
 * @brief 管理新连接的通道信息
 */
void TCommuSvrDisp::AddChannel(TNestChannel* channel)
{
    TNestChannelMap::iterator it = _channel_map.find(channel->GetSockFd());
    if (it != _channel_map.end()) 
    {
        this->DelChannel(it->second);
    }

    _channel_map.insert(TNestChannelMap::value_type(channel->GetSockFd(), channel));
    _un_rigsted.insert(TNestChannelMap::value_type(channel->GetSockFd(), channel));
    
    TNetCommuMng::Instance()->AddNotifyFd(channel->GetSockFd());
}

/**
 * @brief 清理连接的通道信息
 */
void TCommuSvrDisp::DelChannel(TNestChannel* channel)
{
    if (!channel) {
        return;
    }

    // 1. 查询comm server, 删除channel分配
    TNetCommuServer* commu_svr = (TNetCommuServer*)channel->GetCookie();
    if (commu_svr)
    {
        channel->SetCookie(NULL);
        commu_svr->DelChannel(channel);
    }

    // 2. 删除全局的映射, 销毁资源
    _channel_map.erase(channel->GetSockFd());
    _un_rigsted.erase(channel->GetSockFd());  // 冗余删除
    
    TNetCommuMng::Instance()->DelNotifyFd(channel->GetSockFd());
    
    delete channel;
}

/**
 * @brief 注册连接的通道信息
 */
int32_t TCommuSvrDisp::RegistChannel(TNestChannel* channel, uint32_t group)
{
    TNetCommuServer* commu_svr = TNetCommuMng::Instance()->GetCommuServer(group);
    if (!commu_svr) 
    {
        NEST_LOG(LOG_ERROR, "channel route error, fd %d, route id %d", 
            channel->GetSockFd(), group);
        return -1;    
    }

    channel->SetCookie(commu_svr);
    commu_svr->AddChannel(channel);
    _un_rigsted.erase(channel->GetSockFd());

    return 0;
}

/**
 * @brief 管理新连接的通道信息
 */
TNestChannel* TCommuSvrDisp::FindChannel(uint32_t fd)
{
    TNestChannelMap::iterator it = _channel_map.find(fd);
    if (it != _channel_map.end()) 
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief 启动本地侦听, 返回侦听端口
 */
uint32_t TCommuSvrDisp::CheckUnregisted()
{
    TNestChannelMap::iterator it, curr;
    for (it = _un_rigsted.begin(); it != _un_rigsted.end(); /* ++it */)
    {
        curr = it++;  // 先保存当前节点, 再更新迭代器
        
        TNestChannel* channel = curr->second;
        if (!channel)
        {
            NEST_LOG(LOG_ERROR, "invalid channel, error!");
            _un_rigsted.erase(curr);
            continue;
        }
        
        if (channel->RecvCache(false) < 0) 
        {
            NEST_LOG(LOG_ERROR, "recv cache failed, delete channel!");
            this->DoError(channel);    
        }
    }

    return 0;
}

/**
 * @brief 设置通用地址监听, 成功返回0, 失败小于0
 */
int32_t TCommuSvrDisp::StartListen(TNestAddress& address)
{
    _listen_obj.SetDispatch(this);
    _listen_obj.SetListenAddr(&address);
    return _listen_obj.Init();
}

/**
 * @brief 启动本地侦听, 返回侦听端口
 */
uint32_t TCommuSvrDisp::StartListen(uint32_t sid, uint32_t pno, uint32_t addr)
{
    // 1. 默认按id hash处理, 冲突则递增处理
    uint16_t port = (((uint64_t)sid << 32 | pno) % NEST_PROXY_PRIMER) + NEST_PROXY_BASE_PORT;
    struct sockaddr_in  ip_addr; 
    ip_addr.sin_family       =   AF_INET;
    ip_addr.sin_addr.s_addr  =   addr;
    ip_addr.sin_port         =   htons(port);

    _listen_obj.SetDispatch(this);
    _listen_obj.SetListenAddr(&ip_addr);
    if (_listen_obj.Init() >= 0)
    {
        return port;
    }

    // 2. 如果冲突监听, 则暴力尝试
    for (port = NEST_PROXY_BASE_PORT; port < NEST_PROXY_BASE_PORT + 300; port++)
    {
        ip_addr.sin_port         =   htons(port);
        _listen_obj.SetListenAddr(&ip_addr);
        if (_listen_obj.Init() >= 0)
        {
            return port;
        }
    }

    // 3. 失败返回0
    return 0;
}


/**
 * @brief 控制消息处理函数, 虚接口函数, 目前只处理注册消息
 */
void TCommuSvrDisp::HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body)
{
    NEST_LOG(LOG_TRACE, "Server recv ctrl msg, fd %u, type %u, group %u", blob->_recv_fd, 
            head_info.msg_type, head_info.route_id); 
    
    // 1. 获取channel指针, 必须先连接, 后有控制消息
    TNestChannel* channel = this->FindChannel(blob->_recv_fd);
    if (!channel) 
    {
        NEST_LOG(LOG_ERROR, "unknown channel, fd %u, type %u, error", blob->_recv_fd,
                 head_info.msg_type);  
        return;        
    }

    // 2. 分发控制命令处理
    switch (head_info.msg_type)
    {
        case NEST_MSG_TYPE_REGIST:
            NEST_LOG(LOG_DEBUG, "recv channel regist msg, from %s, fd %u.", 
                     channel->GetRemoteAddr().ToString(NULL, 0), blob->_recv_fd);  
            this->RegistChannel(channel, head_info.route_id);
            break;

        case NEST_MSG_TYPE_HEARTBEAT:
            NEST_LOG(LOG_DEBUG, "recv channel heartbeat msg, from %s, fd %u.", 
                     channel->GetRemoteAddr().ToString(NULL, 0), blob->_recv_fd);  
            break;

        default:
            NEST_LOG(LOG_ERROR, "unknown control type, from %s, fd %u, type %u, error", 
                     channel->GetRemoteAddr().ToString(NULL, 0), 
                     blob->_recv_fd, head_info.msg_type);  
            break;
    }
    
    return;
}

/**
 * @brief 业务消息接收后处理函数, 虚接口函数
 */
void TCommuSvrDisp::DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info)
{
    // 如果是无flowid的请求, 不进行统计
    if ((0 == head_info.rcv_sec) && (0 == head_info.rcv_usec)) 
    {
        NEST_LOG(LOG_DEBUG, "unknown context, flow %u, fd %u, type %u, log", 
                 head_info.flow, blob->_recv_fd, head_info.msg_type);  
        return;
    }

    // 计算时间戳信息
    uint64_t time_start = head_info.rcv_sec * 1000 + head_info.rcv_usec / 1000;
    uint64_t time_now   = __spp_get_now_ms();
    uint64_t msg_cost   = (time_now > time_start) ? (time_now - time_start) : 0; 
    
    // 增加统计信息采集, static提高性能
    static CMsgStat* stat = NULL;
    if (!stat) {
        stat = TNetCommuMng::Instance()->GetMsgStat();
    }
    stat->ReportRsp(msg_cost);

    // 计算实际的权重变更
    void* cookie = channel->GetCookie();
    if (cookie)
    {
        TNetCommuServer* commu_svr = (TNetCommuServer*)cookie;
        commu_svr->inc_weight(channel);
        commu_svr->response_report(msg_cost); // 整体的应答统计
    }

    return;
}

/**
 * @brief 初始化监听对象
 */
int32_t TNetCommuServer::init(const void* config)
{
    return 0;
}


/**
 * @brief  外部触发接收, 这里仅返回计数
 */
int32_t TNetCommuServer::poll(bool block)
{
    int32_t process_len = 0;
    TNestChannelList delete_list;
    TNestChannelList::iterator it;
    
    // 1. 遍历各channel, 执行recvcache
    for (it = _channel_list.begin(); it != _channel_list.end(); ++it)
    {
        TNestChannel* channel = *it;
        if (!channel) 
        {
            continue;
        }
        
        int32_t recv = channel->RecvCache(block);
        if (recv < 0)
        {
            delete_list.push_back(channel);     
        }
        else
        {
            process_len += recv;    
        }
    }

    // 2. 清理异常的channel处理
    for (it = delete_list.begin(); it != delete_list.end(); ++it)
    {
        TCommuSvrDisp& disp = TNetCommuMng::Instance()->GetCommuSvrDisp();
        disp.DoError(*it);
    }

    return process_len;
}


/**
 * @brief 执行负载算法, 发送业务报文
 */
int32_t TNetCommuServer::sendto(uint32_t flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    TNestChannel* channel = this->FindNextChannel();
    if (!channel)
    {
        NEST_LOG(LOG_ERROR, "No channel connected, route %d!!", this->GetGroupId());    
        return -1;
    }

    // 封装默认的报文头信息, 扩展字段时, 这里应区分字节流格式 与结构定义
    TNetCommuHead  inner_msg = {0};    
    uint32_t head_len   = sizeof(inner_msg);
    inner_msg.magic_num = NEST_MAGIC_NUM;
    inner_msg.msg_len   = sizeof(inner_msg) + blob->len;
    inner_msg.flow      = flow; 
    inner_msg.head_len  = head_len;
    inner_msg.msg_type  = 0;
    inner_msg.route_id  = _groupid;
    inner_msg.rcv_sec   = __spp_get_now_s();
    inner_msg.rcv_usec  = __spp_g_now_tv.tv_usec;
    inner_msg.crc       = TCommuDisp::generate_crc(&inner_msg);

    TCommuDisp::hton_msg_head(&inner_msg);
    int32_t ret = channel->SendEx((const char*)&inner_msg, head_len, (const char*)blob->data, blob->len);  
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "channel send failed, ret %d!!", ret);    
        return -2;
    }

    // 调整channel的权重信息
    this->dec_weight(channel);

    // 增加统计信息采集, static提高性能
    static CMsgStat* stat = NULL;
    if (!stat) {
        stat = TNetCommuMng::Instance()->GetMsgStat();
    }
    stat->ReportReq();

    return 0;
}

/**
 * @brief 通道管理实现
 */
void TNetCommuServer::AddChannel(TNestChannel* conn)
{
    this->DelChannel(conn);   // 保证无重复添加节点
    
    _channel_list.push_back(conn);
    this->add_item(conn);
}

/**
 * @brief 删除通道的节点
 */
void TNetCommuServer::DelChannel(TNestChannel* conn)
{
    TNestChannelList::iterator it = std::find(_channel_list.begin(), _channel_list.end(), conn);
    if (it != _channel_list.end())
    {
        _channel_list.erase(it);
    }

    this->del_item(conn);
}

/**
 * @brief 按轮训的算法获取下一个通到指针
 */
TNestChannel* TNetCommuServer::FindNextChannel()
{
    if (_channel_list.empty())
    {
        return NULL;
    }

    // 请求触发重算权重逻辑, 10秒调整一次
    uint64_t time_now = __spp_get_now_ms();
    if (time_now > (this->report_start_time() + 10*1000))
    {
        uint32_t dec_step = this->calc_step(time_now, (uint32_t)_channel_list.size());
        this->set_step_params(dec_step);
        this->reset_report(time_now);
        NEST_LOG(LOG_TRACE, "comm server[%u] change step [%u]", _groupid, dec_step);
    }    

    IDispatchAble* item = this->get_next_item();
    if (!item) 
    {
        return NULL;
    }
    else
    {
        this->request_report(); // 统计请求数
        return dynamic_cast<TNestChannel*>(item);
    }
}


/**
 * @brief 连接异常逻辑处理
 */
int32_t TCommuCltDisp::DoError(TNestChannel* channel)
{
    TNetCommuClient* client =  TNetCommuMng::Instance()->GetCommuClient();
    if (!channel || !client) {
        NEST_LOG(LOG_ERROR, "channel %p or client %p null, error!!", channel, client);    
        return -1;
    }

    // 当前单一连接, 直接重连, 多连接时, 映射到commu再重连
    if (!client->RestartChannel())
    {
        NEST_LOG(LOG_ERROR, "channel init failed, error!!");    
        return -2;
    }

    return 0;
}

/**
 * @brief 连接建立通知处理逻辑
 */
int32_t TCommuCltDisp::DoConnected(TNestChannel* channel)
{
    TNetCommuClient* client =  TNetCommuMng::Instance()->GetCommuClient();
    if (!channel || !client) {
        NEST_LOG(LOG_ERROR, "channel %p or client %p null, error!!", channel, client);    
        return -1;
    }

    // 自动发起注册请求, 目前只发一次, 异常等待server的超时断链
    TNetCommuHead  inner_msg = {0};    
    uint32_t head_len   = sizeof(inner_msg);
    inner_msg.magic_num = htonl(NEST_MAGIC_NUM);
    inner_msg.msg_len   = htonl(head_len + 0);
    inner_msg.flow      = 0; 
    inner_msg.head_len  = htonl(head_len);
    inner_msg.msg_type  = htonl(NEST_MSG_TYPE_REGIST);
    inner_msg.route_id  = htonl(client->GetGroupId());
    inner_msg.crc       = TCommuDisp::generate_crc(&inner_msg);

    int32_t ret = channel->Send((const char*)&inner_msg, head_len);  
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "channel send failed, ret %d!!", ret);
        return -2;
    }

    // 设置全局的通知fd
    TNetCommuMng::Instance()->SetClientFd(channel->GetSockFd());
    
    NEST_LOG(LOG_TRACE, "channel regist request ok, group %u!!", client->GetGroupId());    

    return 0;
    
}

/**
 * @brief 业务消息接收后处理函数, 虚接口函数
 */
void TCommuCltDisp::DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info)
{
    // 增加统计信息采集, static提高性能
    static CMsgStat* stat = NULL;
    if (!stat) {
        stat = TNetCommuMng::Instance()->GetMsgStat();
    }
    stat->ReportReq(); 
}

/**
 * @brief 业务消息处理函数, 虚接口函数
 */
void TCommuCltDisp::HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body)
{
    NEST_LOG(LOG_TRACE, "recv service msg, fd %u, type %u, group %u", blob->_recv_fd, 
            head_info.msg_type, head_info.route_id);

    static TNetCommuClient* client = NULL;
    if (!client) {
        client = TNetCommuMng::Instance()->GetCommuClient();
    }

    // 存储上下文信息
    if (!TCommuCltDisp::SaveMsgCtx(head_info))
    {
        NEST_LOG(LOG_ERROR, "ctx save failed, error");
        return;
    }

    // 调用处理消息回调函数
    if (func_list_[CB_RECVDATA]) 
    {
        body.owner = client;
        func_list_[CB_RECVDATA](head_info.flow, &body, func_args_[CB_RECVDATA]);
    } 
    else 
    {
        NEST_LOG(LOG_ERROR, "no recv func regist, error"); 
    }

    return;
}


/**
 * @brief 存储消息信息上下文
 */
bool TCommuCltDisp::SaveMsgCtx(TNetCommuHead& head_info)
{
    static CMsgCtxMng& ctx_mng = TNetCommuMng::Instance()->GetMsgCtxMng();
    CMsgCtx* ctx = ctx_mng.alloc_ctx();
    if (!ctx) {
        NEST_LOG(LOG_ERROR, "alloc ctx failed, error");  
        return false;
    }
    
    ctx->set_msg_head(head_info);
    ctx->set_start_time(__spp_get_now_ms());
    if (!ctx_mng.insert_ctx(ctx)) {
        ctx_mng.free_ctx(ctx);
        NEST_LOG(LOG_ERROR, "insert ctx failed, error");  
        return false;
    }

    return true;
}

/**
 * @brief 获取消息信息上下文
 */
bool TCommuCltDisp::RestoreMsgCtx(uint32_t flow, TNetCommuHead& head_info, uint64_t& save_time)
{
    static CMsgCtxMng& ctx_mng = TNetCommuMng::Instance()->GetMsgCtxMng();
    CMsgCtx* ctx = ctx_mng.find_ctx(flow);
    if (!ctx) {
        NEST_LOG(LOG_DEBUG, "find ctx failed, flow %u", flow);  
        return false;
    }

    head_info = ctx->get_msg_head();
    save_time = ctx->get_start_time();

    ctx_mng.remove_ctx(ctx);
    ctx_mng.free_ctx(ctx);

    return true;
}

/**
 * @brief 构造与析构函数
 */
TNetCommuClient::TNetCommuClient()
{
    _conn_obj = NULL;    
}

TNetCommuClient::~TNetCommuClient()
{
    if (_conn_obj) {
        delete _conn_obj;
        _conn_obj = NULL;
    }
}


/**
 * @brief 初始化连接
 */
TNestChannel* TNetCommuClient::InitChannel()
{
    TNestChannel* channel = new TNestChannel;
    if (!channel)
    {
        NEST_LOG(LOG_ERROR, "channel init failed, error");  
        return NULL;
    }
    
    TCommuCltDisp& disp = TNetCommuMng::Instance()->GetCommuCltDisp();
    channel->SetRemoteAddr(&_net_conf.address);
    channel->SetDispatch(&disp);

    int32_t fd = channel->CreateSock(_net_conf.address.GetAddressDomain());
    if (fd < 0)
    {
        NEST_LOG(LOG_ERROR, "create socket failed, ret %d", fd);
        delete channel;
        return NULL;
    }
    channel->SetSockFd(fd);

    return channel;
}


/**
 * @brief 重启连接
 */
bool TNetCommuClient::RestartChannel()
{
    if (_conn_obj)
    {
        delete _conn_obj;
        _conn_obj = NULL;
    }
    
    _conn_obj = this->InitChannel();
    if (!_conn_obj)
    {
        NEST_LOG(LOG_ERROR, "channel init failed, error!!");    
        return false;
    }

    return true;
}



/**
 * @brief 通道管理的客户端初始化
 */
int32_t TNetCommuClient::init(const void* config)
{
    TNetCoummuConf* conf = (TNetCoummuConf*)config; 
    memcpy(&_net_conf, conf, sizeof(_net_conf));
    
    TNestChannel* channel = this->InitChannel();
    if (!channel)
    {
        NEST_LOG(LOG_ERROR, "channel init failed, error");  
        return -1;
    }

    int32_t ret = channel->DoConnect();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "do connect failed, ret %d", channel->GetSockFd());
        delete channel;
        return -3;
    }

    _conn_obj = channel;
    return 0;
}


/**
 * @brief 通道管理的客户端监控重连逻辑
 */
int32_t TNetCommuClient::CheckConnect()
{
    if (!_conn_obj)
    {
        _conn_obj = this->InitChannel();
        if (!_conn_obj)
        {
            NEST_LOG(LOG_ERROR, "channel init failed, error!!");    
            return -1;
        }
    }

    if (_conn_obj->CheckConnected())
    {
        return 0;
    }

    return _conn_obj->DoConnect();

}


/**
 * @brief 通道管理的客户端主动心跳逻辑
 */
int32_t TNetCommuClient::Heartbeat()
{
    // 1. 检查连接是否存在, 不存在直接失败
    if (NULL == _conn_obj)
    {
        NEST_LOG(LOG_ERROR, "heartbeat, but channel not inited, error!!");    
        return -1;
    }

    // 2. 定制心跳消息发送, 直接失败触发重建, 延迟失败走异常重建
    TNetCommuHead  inner_msg = {0};    
    uint32_t head_len   = sizeof(inner_msg);
    inner_msg.magic_num = htonl(NEST_MAGIC_NUM);
    inner_msg.msg_len   = htonl(head_len + 0);
    inner_msg.flow      = 0; 
    inner_msg.head_len  = htonl(head_len);
    inner_msg.msg_type  = htonl(NEST_MSG_TYPE_HEARTBEAT);
    inner_msg.route_id  = htonl(this->GetGroupId());
    inner_msg.crc       = TCommuDisp::generate_crc(&inner_msg);

    int32_t ret = _conn_obj->Send((const char*)&inner_msg, head_len);  
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "channel send heartbeat failed, ret %d!!", ret);
        return -2;
    }
    
    NEST_LOG(LOG_DEBUG, "channel heartbeat ok, group %u!!", this->GetGroupId());    
    return 0;
}

/**
 * @brief 通道管理的客户端主动心跳逻辑
 */
void TNetCommuClient::CheckHeartbeat()
{
    int32_t ret = this->Heartbeat(); 
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "heartbeat error ret %d, restart connect", ret);
        this->RestartChannel();
        this->CheckConnect();
    }
}


/**
 * @brief 执行epoll收取数据, 返回收取的包数目
 */
int32_t TNetCommuClient::poll(bool block)
{
    static TCommuCltDisp& disp = TNetCommuMng::Instance()->GetCommuCltDisp();
    if (!_conn_obj) 
    {
        NEST_LOG(LOG_DEBUG, "No channel connected, route %d!!", this->GetGroupId());    
        return -1;
    }

    int32_t ret = _conn_obj->RecvCache(false);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "channel recvcache failed, need close! ret %d", ret);
        disp.DoError(_conn_obj);
        return -2;
    }

    return ret;
}


/**
 * @brief 应答报文发送, 捎带负载与时延信息
 */
int32_t TNetCommuClient::sendto(uint32_t flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    if (!_conn_obj)
    {
        NEST_LOG(LOG_ERROR, "No channel connected, route %d!!", this->GetGroupId());    
        return -1;
    }

    // 1. 查询上下文信息
    static TCommuCltDisp& disp = TNetCommuMng::Instance()->GetCommuCltDisp();
    uint64_t time_start = 0;
    uint64_t time_now   = __spp_get_now_ms();
    bool stat_flag = true;
    TNetCommuHead msg_ctx;
    if (!disp.RestoreMsgCtx(flow, msg_ctx, time_start))
    {
        NEST_LOG(LOG_DEBUG, "msg context not find, maybe timeout, flow %u", flow);    
        disp.default_msg_head(&msg_ctx, flow, _net_conf.route_id);
        time_start = __spp_get_now_ms(); // 无上下文, 默认为0时延了
        stat_flag = false;  // 无上下文, 默认不统计请求应答等
    }
    else
    {
        if (time_now == time_start) // 同步逻辑, 不会有时延变化, 重新更新一次时间
        {
            __spp_do_update_tv();
            time_now = __spp_get_now_ms();
        }
    }
    uint64_t msg_cost = (time_now > time_start) ? (time_now - time_start) : 0; 

    if (flow != msg_ctx.flow)
    {
        NEST_LOG(LOG_ERROR, "channel send failed, flow %u, restore %u!!", flow, msg_ctx.flow);    
        return -2;
    }

    // 2. 填充应答信息, 时间消耗等, 时间消耗, 避免反复获取时间戳 同步消息这里不够精确
    msg_ctx.msg_len   = sizeof(msg_ctx) + blob->len;
    msg_ctx.time_cost = msg_cost;
    msg_ctx.crc       = TCommuDisp::generate_crc(&msg_ctx);
    
    TCommuDisp::hton_msg_head(&msg_ctx);
    int32_t ret = _conn_obj->SendEx((const char*)&msg_ctx, sizeof(msg_ctx), (const char*)blob->data, blob->len);  
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "channel send failed, ret %d, route %d!!", ret, _net_conf.route_id);    
        return -3;
    }

    // 兼容接口, 执行发送统计
    if (func_list_[CB_SENDDATA] != NULL)
    {
        func_list_[CB_SENDDATA](flow, blob, func_args_[CB_SENDDATA]);
    }

    // 增加统计信息采集, static提高性能
    static CMsgStat* stat = NULL;
    if (!stat) {
        stat = TNetCommuMng::Instance()->GetMsgStat();
    }
    if (stat_flag) {  // 无上下文, 可能是多次回包, 不再统计
        stat->ReportRsp((uint32_t)msg_cost);
    }

    return 0;
}



/**
 * @brief session全局管理句柄
 * @return 全局句柄指针
 */
TNetCommuMng* TNetCommuMng::_instance = NULL;
TNetCommuMng* TNetCommuMng::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TNetCommuMng();
    }

    return _instance;
}

/**
 * @brief session管理全局的销毁接口
 */
void TNetCommuMng::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}


/**
 * @brief 网络交互连接管理的构造函数
 */
TNetCommuMng::TNetCommuMng()
{
    _client      = NULL;
    _notify_obj  = NULL;
    _client_fd   = 0;
}

/**
 * @brief 析构函数
 */
TNetCommuMng::~TNetCommuMng()
{
    for (TNetCommuSvrMap::iterator it = _server_map.begin(); it != _server_map.end(); ++it)
    {
        delete it->second;
    }    
    _server_map.clear();
    
    _client = NULL;
}


/**
 * @brief proxy负责多个worker组, 按routeid区分获取
 */
TNetCommuServer* TNetCommuMng::GetCommuServer(int32_t route_id) 
{
    if (_server_map.find(route_id) != _server_map.end()) {
        return _server_map[route_id];
    } else {
        return NULL;
    }
}

/**
 * @brief worker目前仅关联一个proxy, 简单获取
 */
TNetCommuClient* TNetCommuMng::GetCommuClient() 
{
    return _client;
}


/**
 * @brief 内部连接客户端注册管理
 */
void TNetCommuMng::RegistClient(TNetCommuClient* client) 
{
    _client = client;
}

/**
 * @brief 内部连接服务端注册管理
 */
void TNetCommuMng::RegistServer(int32_t route, TNetCommuServer* server) 
{
    _server_map[route] = server;
}

/**
 * @brief 内部连接保活
 */
void TNetCommuMng::CheckChannel()
{
    static uint32_t check_count = 0;
    
    // 1. 无注册, 不检查返回
    if (NULL == _client) {
        return;
    }

    // 2. 每周期进行重连确认与重试
    check_count++;
    _client->CheckConnect();

    // 3. 每1W周期进行主动心跳检测
    if ((check_count % NEST_HEARBEAT_TIMES) == 0) {
        _client->CheckHeartbeat();
    }
}

/** 
 * @brief 内部消息上下文队列检测
 */
void TNetCommuMng::CheckMsgCtx(uint64_t now, uint32_t wait)
{
    _msg_ctx.check_expired(now, wait);
}

/**
 * @brief 增加外部的epoll通知机制, 防止外部epoll阻塞
 */
void  TNetCommuMng::AddNotifyFd(int32_t fd)
{
    if (!_notify_obj) {
        NEST_LOG(LOG_ERROR, "notify obj null, error!!");    
        return;
    }

    CTSockCommu* sock_commu = (CTSockCommu*)_notify_obj;
    int32_t ret = sock_commu->add_notify(fd);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "notify obj add failed, fd %d, ret %d, error!!", fd, ret);    
    }

    return;
}

/**
 * @brief 增加外部的epoll通知机制, 防止外部epoll阻塞
 */
void TNetCommuMng::DelNotifyFd(int32_t fd)
{
    if (!_notify_obj) {
        NEST_LOG(LOG_ERROR, "notify obj null, error!!");    
        return;
    }

    CTSockCommu* sock_commu = (CTSockCommu*)_notify_obj;
    int32_t ret = sock_commu->del_notify(fd);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "notify obj add failed, fd %d, ret %d, error!!", fd, ret);    
    }

    return;
}


