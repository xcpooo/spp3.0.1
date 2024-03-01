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
 * @brief �������ȫ�ֹ�����
 * @return ȫ�־��ָ��
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
 * @brief ����ȫ�ֵ����ٽӿ�
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
 * @brief ����unix domain socket path
 */
void TAgentNetMng::RmUnixSockPath(uint32_t pid)
{
    char sun_path[256];
    snprintf(sun_path,  sizeof(sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
    unlink(sun_path);
}


/**
 * @brief ���unix domain socket path
 */
bool TAgentNetMng::CheckUnixSockPath(uint32_t pid, bool abstract)
{
    // 1. �ǳ���ĵ�ַ, ֱ�Ӽ��·��
    if (!abstract) {
        char sun_path[256];
        snprintf(sun_path,  sizeof(sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
        return TAgentNetMng::IsFileExist(sun_path);
    }

    // 2. �����ַ, ���Է�0��̽��(���;��ɳɹ�,tcp�ղ���,udp���յ�)
    TNestAddress address;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), NEST_PROC_UNIX_PATH"_%u", pid);
    address.SetAddress(&uaddr, true);
    return (0 == TAgentNetMng::Instance()->SendtoLocal(&address, NULL, 0));
}


/**
 * @brief �ж�Ŀ¼�Ƿ����
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
 * @brief �ж��ļ��Ƿ����
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
 * @brief �񳲴������ӹ�����������
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
 * @brief �񳲴������ӹ�������
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
 * @brief �񳲴������ӽ�����ʼ��
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
 * @brief �񳲴������ӳ��Ը���server
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
 * @brief ͨ������Ŀͻ��˼�������߼�
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

    // 1. ȷ���������, ȷ���Ƿ���IP����
    if (_keep_conn->CheckConnected())
    {
        return 0;
    }

    if (this->_server_addrs.empty()) 
    {
        NEST_LOG(LOG_TRACE, "channel no server to connect, failed");    
        return -2;
    }

    // 2. ���Դ����ж�
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
 * @brief ͨ������Ĳ�ѯ����������Ϣ
 */
TNestChannel* TAgentNetMng::FindChannel(uint32_t fd)
{
    // 1. ���Ȳ���map
    TNestChannelMap::iterator it = _channel_map.find(fd);
    if (it != _channel_map.end())
    {
        return it->second;
    }

    // 2. ȷ���Ƿ�keep
    if ((_keep_conn) && ((int32_t)fd == _keep_conn->GetSockFd()))
    {
        return _keep_conn;
    }

    // 3. û�ҵ�channel
    return NULL;
}

/**
 * @brief ��ʼ�� Agent ���Ӷ���
 */
int32_t TAgentNetMng::InitService(uint32_t setid, TNestAddrArray& servers)
{
    // 1. ��������, �������
    _setid          = setid;
    _server_addrs   = servers;
    std::srand(time(NULL));
    std::random_shuffle(_server_addrs.begin(), _server_addrs.end());
    
    // 1. tcp ������ʼ��
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

    // 2. unix ����socket ��ʼ��
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

    // 3. ��ʼ�����Ӷ���
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

    // 4. conn �������Ӷ���
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
 * @brief ����setid�����Ӷ���
 */
int32_t TAgentNetMng::UpdateSetInfo(uint32_t setid, TNestAddrArray& servers)
{
    // 1. ��������, �������һ��
    _setid          = setid;
    _server_addrs   = servers;
    std::random_shuffle(_server_addrs.begin(), _server_addrs.end());

    // 2. ȷ�ϵ�ǰ���ӵ�server�Ƿ���Ч
    if (std::find(_server_addrs.begin(), _server_addrs.end(), _server)
        != _server_addrs.end())
    {
        NEST_LOG(LOG_TRACE, "Curr server in new list ok");
        return 0;
    }

    // 3. �л�server, ��������
    if (_keep_conn) {
        delete _keep_conn;
        _keep_conn = NULL;
    }
    if (!_server_addrs.empty())
    {
        _server = _server_addrs[0];
        _server_index = 0;
    }
    
    // 4. ���ȱ���
    _retry_count = 0; 
    this->CheckKeepAlive();

    return 0;    
}


/**
 * @brief ����service, Ĭ��epoll�ȴ�10ms
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
 * @brief ������ձ���, �������
 */
int32_t TAgentNetMng::DoRecv(TNestBlob* blob)
{
    char* pos = (char*)blob->_buff;
    uint32_t left = blob->_len;
    string err_msg;

    for ( ; ; )
    {
        // 1. ��鱨�ĺϷ���
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
        
        // 2. ��������ͷ��Ϣ
        nest_msg_head head;
        if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len))
        {
            NEST_LOG(LOG_ERROR, "pkg head parse failed, log");    
            continue;
        }       

        // 3. �ַ�����ӿ�
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
 * @brief �������ӽ�������
 */
int32_t TAgentNetMng::DoAccept(int32_t cltfd, TNestAddress* addr)
{
    // 1. ��������, ����״̬
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

    // 2. �����¼�ע��
    client->EnableInput();
    client->DisableOutput();
    if (client->AttachPoller(TNestNetComm::Instance()->GetPollUnit()) < 0)
    {
        NEST_LOG(LOG_ERROR, "Tcp channel event attach failed, fd %d", cltfd);    
        close(cltfd);
        delete client;
        return -2;
    }

    // 3. ����������Ϣ
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
 * @brief ���������쳣��Ϣ���
 */
int32_t TAgentNetMng::DoError(TNestChannel* channel)
{
    // 1. ȷ���Ƿ�ΪLISTEN
    if (_keep_conn == channel)
    {
        delete _keep_conn;
        _keep_conn = NULL;

        return 0;
    }

    // 2. ȷ���Ƿ�Ϊ����Զ�̽ڵ�
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

    // 3. server �����쳣, ����
    return 0;

}

/**
 * @brief �򱾵ص�ַ���Ͳ��ȴ�Ӧ����Ϣ
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

    // ����CLOEXEC
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

    // ���ͳ�ʱ�̶�10ms, ���ճ�ʱ��ʵ�����
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
 * @brief �򱾵ص�ַ���Ͳ��ȴ�Ӧ�����Ϣ
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

    // ����CLOEXEC
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
 * @brief �򱾵�agent������Ϣ
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
 * @brief �������ӵ�server������Ϣ
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





