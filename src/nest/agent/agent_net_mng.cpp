/**
 * @brief Nest agent net manager
 */
#include <stdint.h>
#include <sys/syscall.h>
#include "nest_log.h"
#include "agent_msg.h"
#include "agent_net_mng.h"
#include "nest_agent.h"
#include <sys/types.h>
#include <dirent.h>

#ifdef MAXPATHLEN
#undef MAXPATHLEN
#endif

#include "misc.h"

using namespace nest;


/**
 * @brief 网络管理全局管理句柄
 * @return 全局句柄指针
 */
TAgentNetMng* TAgentNetMng::_instance = NULL;
TAgentNetMng* TAgentNetMng::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TAgentNetMng();
    }

    return _instance;
}

/**
 * @brief 管理全局的销毁接口
 */
void TAgentNetMng::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief 清理unix domain socket path
 */
void TAgentNetMng::RmUnixSockPath(uint32_t pid)
{
    char sun_path[256];
    snprintf(sun_path,  sizeof(sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
    unlink(sun_path);
}


/**
 * @brief 检查unix domain socket path
 */
bool TAgentNetMng::CheckUnixSockPath(uint32_t pid, bool abstract)
{
    // 1. 非抽象的地址, 直接检查路径
    if (!abstract) {
        char sun_path[256];
        snprintf(sun_path,  sizeof(sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
        return TAgentNetMng::IsFileExist(sun_path);
    }

    // 2. 抽象地址, 尝试发0包探测(发送均可成功,tcp收不到,udp可收到)
    TNestAddress address;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
    address.SetAddress(&uaddr, true);
    return (0 == TAgentNetMng::Instance()->SendtoLocal(&address, NULL, 0));
}


/**
 * @brief 判定目录是否存在
 */
bool TAgentNetMng::IsDirExist(const char* path)
{
    if (!path) {
        return false;
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    } else {
        closedir(dir);
        return true;
    } 
}

/**
 * @brief 判定文件是否存在
 */
bool TAgentNetMng::IsFileExist(const char* path)
{
    if (!path) {
        return false;
    }
    
    int32_t result = access (path, F_OK);
    if (result < 0) {
        return false;
    } else {
        return true;
    } 
}

/**
 * @brief 鸟巢代理连接管理构造与析构
 */
TAgentNetMng::TAgentNetMng()
{
    _keep_conn      = NULL;
    _tcp_listen     = new TNestListen;
    _unix_listen    = new TNestUdpListen;
    _setid          = 0;
    _retry_count    = 0;
    _server_index   = 0;

    _tcp_listen->SetDispatch(this);
    _unix_listen->SetDispatch(this);
}

/**
 * @brief 鸟巢代理连接管理析构
 */
TAgentNetMng::~TAgentNetMng()
{
    if (_keep_conn) {
        delete _keep_conn;
        _keep_conn = NULL;
    }

    if (_tcp_listen) {
        delete _tcp_listen;
        _tcp_listen = NULL;
    }

    if (_unix_listen) {
        delete _unix_listen;
        _unix_listen = NULL;
    }

    for (TNestChannelMap::iterator it = _channel_map.begin(); 
            it != _channel_map.end(); ++it)
    {
        delete it->second;
    }
    
}

/**
 * @brief 鸟巢代理长连接建立初始化
 */
TNestChannel* TAgentNetMng::InitKeepChannel()
{
    TNestChannel* channel = new TNestChannel;
    if (!channel)
    {
        NEST_LOG(LOG_ERROR, "channel init failed, error");  
        return NULL;
    }
    channel->SetDispatch(this);

    int32_t fd = channel->CreateSock(AF_INET);
    if (fd < 0)
    {
        NEST_LOG(LOG_ERROR, "create socket failed, ret %d", fd);
        delete channel;
        return NULL;
    }
    channel->SetSockFd(fd);    
    fcntl(fd, F_SETFD, FD_CLOEXEC);

    return channel;
}


/**
 * @brief 鸟巢代理长连接尝试更换server
 */
void TAgentNetMng::ChangeServerAddr()
{
    if (this->_server_addrs.empty()) {
        return;
    }

    _server_index++;
    if ((_server_index < 0) || (_server_index >= _server_addrs.size()))
    {
        _server_index = 0;
    } 

    _server = _server_addrs[_server_index];
}


/**
 * @brief 通道管理的客户端监控重连逻辑
 */
int32_t TAgentNetMng::CheckKeepAlive()
{
    if (!_keep_conn)
    {
        _keep_conn = this->InitKeepChannel();
        if (!_keep_conn)
        {
            NEST_LOG(LOG_ERROR, "channel init failed, error!!");    
            return -1;
        }
    }

    // 1. 确认连接与否, 确认是否有IP可连
    if (_keep_conn->CheckConnected())
    {
        return 0;
    }

    if (this->_server_addrs.empty()) 
    {
        NEST_LOG(LOG_TRACE, "channel no server to connect, failed");    
        return -2;
    }

    // 2. 重试次数判定
    if (this->_retry_count > 10)
    {
        this->ChangeServerAddr();
        this->_retry_count = 0;
    }
    
    _keep_conn->SetRemoteAddr(&_server);
    this->_retry_count++;   

    return _keep_conn->DoConnect();

}


/**
 * @brief 通道管理的查询已有连接信息
 */
TNestChannel* TAgentNetMng::FindChannel(uint32_t fd)
{
    // 1. 优先查找map
    TNestChannelMap::iterator it = _channel_map.find(fd);
    if (it != _channel_map.end())
    {
        return it->second;
    }

    // 2. 确认是否keep
    if ((_keep_conn) && ((int32_t)fd == _keep_conn->GetSockFd()))
    {
        return _keep_conn;
    }

    // 3. 没找到channel
    return NULL;
}

/**
 * @brief 初始化 Agent 连接对象
 */
int32_t TAgentNetMng::InitService(uint32_t setid, TNestAddrArray& servers)
{
    // 1. 更新配置, 随机处理
    _setid          = setid;
    _server_addrs   = servers;
    std::srand(time(NULL));
    std::random_shuffle(_server_addrs.begin(), _server_addrs.end());
    
    // 1. tcp 侦听初始化
    struct sockaddr_in iaddr;
    iaddr.sin_family       =   AF_INET;
    iaddr.sin_addr.s_addr  =   spp::comm::CMisc::getip("eth1");
    iaddr.sin_port         =   htons(NEST_AGENT_PORT);
    _tcp_listen->SetListenAddr(&iaddr);
    int32_t ret = _tcp_listen->Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "Init tcp listen failed, ret %d", ret);
        return ret;
    }
    fcntl(_tcp_listen->GetSockFd(), F_SETFD, FD_CLOEXEC);

    // 2. unix 域报文socket 初始化
    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_AGENT_UNIX_PATH);
    _unix_listen->SetListenAddr(&uaddr, true);
    ret = _unix_listen->Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "Init unix datagram listen failed, ret %d", ret);
        return ret;
    }
    fcntl(_unix_listen->GetSockFd(), F_SETFD, FD_CLOEXEC);

    // 3. 初始化连接对象
    _keep_conn = this->InitKeepChannel();
    if (!_keep_conn)
    {
        NEST_LOG(LOG_ERROR, "Init client keep failed, ret %d", ret);
        return ret;
    }

    if (servers.empty()) 
    {
        NEST_LOG(LOG_TRACE, "Init no set info wait");
        return 0;
    }
    _server = _server_addrs[0];

    // 4. conn 发起连接对象
    _keep_conn->SetRemoteAddr(&_server);
    ret = _keep_conn->DoConnect();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "client keep connect failed, ret %d", ret);
        return ret;        
    }

    return 0;
}


/**
 * @brief 更新setid与连接对象
 */
int32_t TAgentNetMng::UpdateSetInfo(uint32_t setid, TNestAddrArray& servers)
{
    // 1. 更新配置, 随机处理一下
    _setid          = setid;
    _server_addrs   = servers;
    std::random_shuffle(_server_addrs.begin(), _server_addrs.end());

    // 2. 确认当前连接的server是否有效
    if (std::find(_server_addrs.begin(), _server_addrs.end(), _server)
        != _server_addrs.end())
    {
        NEST_LOG(LOG_TRACE, "Curr server in new list ok");
        return 0;
    }

    // 3. 切换server, 发起重连
    if (_keep_conn) {
        delete _keep_conn;
        _keep_conn = NULL;
    }
    if (!_server_addrs.empty())
    {
        _server = _server_addrs[0];
        _server_index = 0;
    }
    
    // 4. 调度保活
    _retry_count = 0; 
    this->CheckKeepAlive();

    return 0;    
}


/**
 * @brief 运行service, 默认epoll等待10ms
 */
void TAgentNetMng::RunService()
{
    CPollerUnit* poller = TNestNetComm::Instance()->GetPollUnit();
    if (!poller)
    {
        NEST_LOG(LOG_ERROR, "Commu poll not init, error!!");    
        return;
    }

    poller->WaitPollerEvents(10);
    poller->ProcessPollerEvents();
}

/**
 * @brief 处理接收报文, 命令解析
 */
int32_t TAgentNetMng::DoRecv(TNestBlob* blob)
{
    char* pos = (char*)blob->_buff;
    uint32_t left = blob->_len;
    string err_msg;

    for ( ; ; )
    {
        // 1. 检查报文合法性
        NestPkgInfo pkg_info = {0};
        pkg_info.msg_buff  = pos;
        pkg_info.buff_len  = left;
    
        int32_t ret = NestProtoCheck(&pkg_info, err_msg);
        if (ret == 0) 
        {
            NEST_LOG(LOG_TRACE, "pkg need wait more, len %u", left);    
            break;
        }
        else if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "pkg parse error, ret %d, msg: %s", ret, err_msg.c_str());    
            return -1;
        }
        
        pos  += ret;
        left -= ret;
        
        // 2. 解析报文头信息
        nest_msg_head head;
        if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len))
        {
            NEST_LOG(LOG_ERROR, "pkg head parse failed, log");    
            continue;
        }       

        // 3. 分发处理接口
        NestMsgInfo  msg;
        msg.blob_info   = blob;
        msg.pkg_info    = &pkg_info;
        msg.nest_head   = &head;
        ret = CAgentProxy::Instance()->DispatchMsg(&msg);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "pkg dispatch process failed, ret %d", ret);    
            continue;
        }
    }

    return (int32_t)(pos - (char*)blob->_buff);
}


/**
 * @brief 处理连接建立过程
 */
int32_t TAgentNetMng::DoAccept(int32_t cltfd, TNestAddress* addr)
{
    // 1. 创建对象, 设置状态
    TNestChannel* client = new TNestChannel;
    if (!client)
    {
        NEST_LOG(LOG_ERROR, "Tcp channel alloc failed, fd %d", cltfd);    
        close(cltfd);
        return -1;
    }
    client->SetSockFd(cltfd);
    client->SetRemoteAddr(addr);
    client->SetConnState(CONNECTED);
    client->SetDispatch(this);

    // 2. 监听事件注册
    client->EnableInput();
    client->DisableOutput();
    if (client->AttachPoller(TNestNetComm::Instance()->GetPollUnit()) < 0)
    {
        NEST_LOG(LOG_ERROR, "Tcp channel event attach failed, fd %d", cltfd);    
        close(cltfd);
        delete client;
        return -2;
    }

    // 3. 管理连接信息
    TNestChannelMap::iterator it = _channel_map.find(cltfd);
    if (it != _channel_map.end())
    {
        NEST_LOG(LOG_ERROR, "Tcp channel manager error, repeat fd: %d", cltfd);
        _channel_map.erase(it);
        delete it->second;
    }
    _channel_map[cltfd] = client;
    fcntl(cltfd, F_SETFD, FD_CLOEXEC);

    return 0;

}


/**
 * @brief 处理连接异常信息情况
 */
int32_t TAgentNetMng::DoError(TNestChannel* channel)
{
    // 1. 确认是否为LISTEN
    if (_keep_conn == channel)
    {
        delete _keep_conn;
        _keep_conn = NULL;

        return 0;
    }

    // 2. 确认是否为接入远程节点
    TNestChannelMap::iterator it = _channel_map.find(channel->GetSockFd());
    if (it != _channel_map.end())
    {
        _channel_map.erase(it);
        delete it->second;
    }
    else
    {
        NEST_LOG(LOG_ERROR, "Tcp channel not in manager %d", channel->GetSockFd());
        delete channel;
    }

    // 3. server 监听异常, 忽略
    return 0;

}

/**
 * @brief 向本地地址发送并等待应答消息
 */
int32_t TAgentNetMng::SendRecvLocal(TNestAddress* addr, const void* data, uint32_t len, 
                          void* buff, uint32_t& buff_len, uint32_t time_wait)
{
    int32_t sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        NEST_LOG(LOG_ERROR, "socket create failed(%m)");
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), NEST_THREAD_UNIX_PATH"_%u", (uint32_t)syscall(__NR_gettid));
    
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), "%s", path);
    socklen_t addr_len = SUN_LEN(&uaddr);
    uaddr.sun_path[0] = '\0';
    if (bind(sockfd, (struct sockaddr*)&uaddr, addr_len) < 0)
    {
        NEST_LOG(LOG_ERROR, "socket bind failed(%m)");
        close(sockfd);
        return -1;
    }

    // 设置CLOEXEC
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

    // 发送超时固定10ms, 接收超时按实际情况
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10 * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    tv.tv_sec = time_wait / 1000;
    tv.tv_usec = (time_wait % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int32_t bytes = sendto(sockfd, data, len, 0, addr->GetAddress(), addr->GetAddressLen());
    if (bytes < 0)
    {
        NEST_LOG(LOG_ERROR, "socket send failed, dst %s(%m)", addr->ToString(NULL, 0));
        close(sockfd);
        return -2;
    }

    bytes = recvfrom(sockfd, buff, buff_len, 0, NULL, 0);
    if (bytes < 0)
    {
        NEST_LOG(LOG_ERROR, "socket recv failed(%m)");
        close(sockfd);
        return -3;
    }

    buff_len = bytes;
    close(sockfd);
    return bytes;   
}                          

/**
 * @brief 向本地地址发送不等待应答的消息
 */
int32_t TAgentNetMng::SendtoLocal(TNestAddress* addr, const void* data, uint32_t len)
{
    int32_t sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        NEST_LOG(LOG_ERROR, "socket create failed(%m)");
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), NEST_THREAD_UNIX_PATH"_%u", (uint32_t)syscall(__NR_gettid));
    
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), "%s", path);
    socklen_t addr_len = SUN_LEN(&uaddr);
    uaddr.sun_path[0] = '\0';
    if (bind(sockfd, (struct sockaddr*)&uaddr, addr_len) < 0)
    {
        NEST_LOG(LOG_ERROR, "socket bind failed(%m)");
        close(sockfd);
        return -1;
    }

    // 设置CLOEXEC
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int32_t bytes = sendto(sockfd, data, len, 0, addr->GetAddress(), addr->GetAddressLen());
    if (bytes < 0)
    {
        NEST_LOG(LOG_ERROR, "socket send failed(%m)");
        close(sockfd);
        return -2;
    }

    close(sockfd);
    return bytes;
}


/**
 * @brief 向本地agent发送消息
 */
int32_t TAgentNetMng::SendtoAgent(const void* data, uint32_t len)
{
    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_AGENT_UNIX_PATH);

    TNestAddress address;
    address.SetAddress(&uaddr, true);

    return SendtoLocal(&address, data, len);
}

/**
 * @brief 向已连接的server发送消息
 */
int32_t TAgentNetMng::SendToServer(void* buff, uint32_t len)
{
    if (!_keep_conn || !_keep_conn->CheckConnected())
    {
        NEST_LOG(LOG_ERROR, "Nest agent not connected!");
        return -1;
    }

    return _keep_conn->Send((const char*)buff, len);
}





