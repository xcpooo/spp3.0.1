/**
 * @brief Nest agent net dispatcher
 */
#include <stdint.h>
#include <sys/syscall.h>
#include "nest_log.h"
#include "agent_msg.h"
#include "agent_net_dispatch.h"
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
TAgentNetDispatch* TAgentNetDispatch::_instance = NULL;
TAgentNetDispatch* TAgentNetDispatch::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TAgentNetDispatch();
    }

    return _instance;
}

/**
 * @brief ����ȫ�ֵ����ٽӿ�
 */
void TAgentNetDispatch::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}


/**
 * @brief �񳲴������ӹ�����������
 */
TAgentNetDispatch::TAgentNetDispatch()
{
        _msg_dispatch = NULL;
}

/**
 * @brief �񳲴������ӹ�������
 */
TAgentNetDispatch::~TAgentNetDispatch()
{
    for (TNestChannelMap::iterator it = _channel_map.begin(); 
            it != _channel_map.end(); ++it)
    {
        if(it->second!=NULL)
        {
            delete it->second;
        }
    }

    for(TNestListenMap::iterator itL = _listen_map.begin();
            itL != _listen_map.end(); ++itL)
    {
        if(itL->second!=NULL)
        {
            delete itL->second;
        }
    }
}


/**
 * @brief ͨ������Ĳ�ѯ����������Ϣ
 */
TNestChannel* TAgentNetDispatch::FindChannel(uint32_t fd)
{
    // 1. ���Ȳ���map
    TNestChannelMap::iterator it = _channel_map.find(fd);
    if (it != _channel_map.end())
    {
        return it->second;
    }

    // 2. û�ҵ�channel
    return NULL;
}


/**
 * @brief ��Ӽ����˿�
 */
int32_t TAgentNetDispatch::AddListen(const TNestAddress& addr, bool tcp)
{
    // ����Ƿ��Ѿ���
    TNestListenMap::iterator it = _listen_map.begin();
    TNestListen *listen = NULL;
    const char * addr_str = const_cast<TNestAddress&>(addr).ToString(NULL,0);
    const char * proto_str = (tcp?"tcp":"udp");
    for(;it!=_listen_map.end();++it)
    {
        listen = it->second;
        if(listen==NULL)
        {
            continue;
        }

        // �Ѿ���
        if(listen->IsTcp()==tcp && listen->GetListenAddr()==addr)
        {
            NEST_LOG(LOG_TRACE, "add %s listen at (%s) ok, already listened",proto_str, addr_str);
            return 0;
        }
    }

    if(tcp)
    {
        listen = new TNestListen();
    }
    else
    {
        listen = new TNestUdpListen();
    }

    if(listen==NULL)
    {
        NEST_LOG(LOG_ERROR, "add %s listen at (%s) failed, new Listen object failed, (%m)", proto_str,addr_str);
        return 0;        
    }

    listen->SetDispatch(this);
    listen->SetListenAddr(const_cast<TNestAddress*>(&addr));
    
    int32_t ret = listen->Init();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "Init %s listen at (%s) failed, ret %d, (%m)",  proto_str,addr_str,ret);
        delete listen;
        return ret;
    }

    fcntl(listen->GetSockFd(), F_SETFD, FD_CLOEXEC);

    _listen_map.insert(make_pair(listen->GetSockFd(), listen));

    NEST_LOG(LOG_TRACE, "add %s listen at (%s) ok", proto_str, addr_str);

    return 0;
}

/**
 * @brief ������Ϣ�������
 */
int32_t TAgentNetDispatch::SetMsgDispatch(TMsgDispatch *msg_dispatch)
{
    if(msg_dispatch==NULL)
    {
        NEST_LOG(LOG_ERROR, "msg dispatch input param is null");
        return -1;        
    }
    
    if(_msg_dispatch!=NULL)
    {
        NEST_LOG(LOG_ERROR, "msg dispatch already set");
        return -1;
    }

    _msg_dispatch = msg_dispatch;
    return 0;
}


/**
 * @brief ����service, Ĭ��epoll�ȴ�10ms
 */
void TAgentNetDispatch::RunService()
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
int32_t TAgentNetDispatch::DoRecv(TNestBlob* blob)
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

        if(_msg_dispatch!=NULL)
        {
            ret = _msg_dispatch->dispatch((void*)&msg);
        }        
        
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
int32_t TAgentNetDispatch::DoAccept(int32_t cltfd, TNestAddress* addr)
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
int32_t TAgentNetDispatch::DoError(TNestChannel* channel)
{
    // 1. ȷ���Ƿ�Ϊ����Զ�̽ڵ�
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

    // 2. server �����쳣, ����
    return 0;

}






