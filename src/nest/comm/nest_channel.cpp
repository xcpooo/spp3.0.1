/**
 * @brief net commu for proxy worker
 */

#include <errno.h>
#include <sys/ioctl.h>
#include "global.h"
#include "nest_channel.h"
#include "nest_log.h"

using namespace nest;
using namespace tbase::tcommu::tsockcommu;

/**
 * @brief ��������������
 */
TNestChannel::TNestChannel()
{
    _cc = new ConnCache(*TNestNetComm::Instance()->GetMemPool());
    netfd           = -1;
    _conn_state     = UNCONNECTED;
    _dispatch       = NULL;
    _cookie         = NULL;
}

TNestChannel::~TNestChannel()
{
    if (_cc) {
        delete _cc;
    }

    if (netfd != -1) {
        close(netfd);
        netfd = -1;
    }
}


/**
 * @brief ������������socket
 * @return >=0 fd, < 0 ʧ��
 */
int32_t TNestChannel::CreateSock(int32_t domain, int32_t type)
{
    int32_t fd = socket(domain, type, 0);
    if (fd < 0)
    {
        NEST_LOG(LOG_ERROR, "create tcp socket failed, error: %d(%m)", errno);
        return -1;
    }
    
    int flags = 1;
    if (ioctl(fd, FIONBIO, &flags) < 0)
    {
        NEST_LOG(LOG_ERROR, "set tcp socket unblock failed, error: %d(%m)", errno);
        close(fd);
        fd = -1;
        return -2;
    }

    int optval = 1;
    unsigned optlen = sizeof(optval);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    return fd;
}


/**
 * @brief �ͻ���������������
 * @return >0 ���ӳɹ�, <0 ����ʧ��, =0 ���ӽ�����
 */
int32_t TNestChannel::Connect()
{
    int ret = 0;
    int rc = ::connect(netfd, _remote_addr.GetAddress(), _remote_addr.GetAddressLen());
    if (rc < 0)
    {
        int32_t err = errno;
        if (err == EISCONN)
        {
            ret = 1;
        }
        else
        {
            if ((err == EINPROGRESS) || (err == EALREADY) || (err == EINTR))
            {
                NEST_LOG(LOG_TRACE, "Open connect not ok, maybe first try, sock %d, dst %s, errno %d(%m),",
                         netfd, _remote_addr.ToString(NULL, 0), err);
                ret = 0;
            }
            else
            {
                NEST_LOG(LOG_ERROR, "Open connect not ok, sock %d, dst %s, errno %d(%m)", netfd,
                        _remote_addr.ToString(NULL, 0), err);
                ret = -1;
            }
        }
    }
    else
    {
        ret = 1;
    }

    return ret;

}

/**
 * @brief ������ӽ���, ��ִ�����ӹ���, �����¼�����
 * @return < 0 ʧ��, > 0 ������, =0 ������
 */
int32_t TNestChannel::DoConnect()
{
    bool connected = false;
    
    if (this->CheckConnected()) 
    {
        NEST_LOG(LOG_DEBUG, "socket [%d] connect already", netfd);
        return 1;
    }
    
    int32_t ret = this->Connect();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "socket connect failed, ret %d", netfd);
        return -1;
    }
    else
    {
        if (ret > 0)
        {
            this->SetConnState(CONNECTED);
            this->EnableInput();
            this->DisableOutput();
            connected = true;

            NEST_LOG(LOG_TRACE, "socket connect ok, send regist, fd %d", netfd);
            if (_dispatch)
            {
                _dispatch->DoConnected(this);
            }
        }
        else
        {
            this->SetConnState(CONNECTING);
            this->DisableInput();
            this->EnableOutput();
        }
        
        int32_t rc = 0;
        if (this->GetPollerUnit() == NULL)
        {
            rc = this->AttachPoller(TNestNetComm::Instance()->GetPollUnit());   
        }
        else
        {
            rc = this->ApplyEvents();
        }

        if (rc < 0)
        {
            NEST_LOG(LOG_ERROR, "events attach failed, fd %d, %m", this->GetSockFd());
            return -2;
        }
        
        return connected ? 1 : 0;
    }
}





/**
 * @brief ��ͨ����ȡ����, ��ʱ�������ݽ����������
 * @return >0 �ɹ�,���ؽ��յ��ֽ�; <0 ����ʧ��, =0 �Զ˹ر�
 */
int32_t TNestChannel::Recv()
{
    static char buf[1024*1024];
    uint32_t process_len = 0;
    
    int bytes = ::recv(netfd, buf, sizeof(buf), 0);
    if (bytes > 0)
    {
        _cc->_r.append(buf, bytes);

        TNestBlob  blob;
        blob._type = NEST_BLOB_TYPE_STREAM;
        blob._buff = _cc->_r.data();
        blob._len  = _cc->_r.data_len();
        blob._recv_fd = netfd;
        blob._remote_addr = _remote_addr;

        if (!_dispatch)
        {
            NEST_LOG(LOG_ERROR, "socket[%d] no dispatch, error", netfd);
            return -100;
        }

        process_len = _dispatch->DoRecv(&blob);
        if (process_len < 0)
        {
            NEST_LOG(LOG_ERROR, "socket[%d] dispatch recv, error[%d]", netfd, process_len);
            return -200;
        }
        
        _cc->_r.skip(process_len);
        return process_len;
    }
    else if (bytes == 0)
    {
        NEST_LOG(LOG_TRACE, "remote close socket[%d] remote[%s]", netfd, 
            _remote_addr.ToString(NULL, 0));
        this->RecvCache(true);  // ��������buff
        return -102;
    }
    else
    {
        NEST_LOG(LOG_ERROR, "socket[%d] recv error; remote[%s], errno %d(%m) ", netfd, 
            _remote_addr.ToString(NULL, 0), errno);
        return -103;
    }
}

/**
 * @brief ����ÿ��ֻ����һ�����ĵ�����, �ṩ�����ȡ�ӿ�
 * @param  block �Ƿ�����, true��һ��ȡ��, false ��ÿ��ȡһ��
 * @return ������ֽ���, С��0ʧ��
 */
int32_t TNestChannel::RecvCache(bool block)
{
    uint32_t process_len = 0;

    if (_cc->_r.data_len() == 0) {
        return 0;
    }

    TNestBlob  blob;
    blob._type = NEST_BLOB_TYPE_STREAM;
    blob._buff = _cc->_r.data();
    blob._len  = _cc->_r.data_len();
    blob._recv_fd = netfd;
    blob._remote_addr = _remote_addr;

    if (!_dispatch)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] no dispatch, error", netfd);
        return -100;
    }

    process_len = _dispatch->DoRecvCache(&blob, this, block);
    if (process_len < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] dispatch recv, error[%d]", netfd, process_len);
        return -200;
    }
    
    _cc->_r.skip(process_len);
    
    return process_len;
}


/**
 * @brief ��TCPͨ��д������, ����ʣ����ֶ�, ����д������
 * @return >=0 �ɹ�,���ط��͵��ֽ�; <0 ʧ��
 */
int32_t TNestChannel::Send(const char* data, uint32_t data_len)
{
    int32_t bytes = SendEx(NULL, 0, data, data_len);
    if (bytes < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] send error; remote[%s], errno %d(%m) ", netfd, 
            _remote_addr.ToString(NULL, 0), errno);

        if (_dispatch)
        {
            _dispatch->DoError(this);
        } 
        else 
        {
            NEST_LOG(LOG_ERROR, "socket[%d] no dispatch, error", netfd);
        }
    }

    return bytes;
}

/**
 * @brief ��TCPͨ��д������, ����ʣ����ֶ�, ����д������
 * @return >=0 �ɹ�,���ط��͵��ֽ�; <0 ʧ��
 */
int32_t TNestChannel::SendEx(const char* ctrl, uint32_t ctrl_len, const char* data, uint32_t data_len)
{
    struct iovec iov[3];
    iov[0].iov_base = _cc->_w.data();
    iov[0].iov_len  = _cc->_w.data_len();
    iov[1].iov_base = (void*)ctrl;
    iov[1].iov_len  = ctrl_len;
    iov[2].iov_base = (void*)data;
    iov[2].iov_len  = data_len;

    ssize_t bytes = writev(netfd, iov, 3);
    if (bytes < 0)
    {
        if ((errno == EAGAIN) || (errno == EINTR))
        {
            _cc->_w.append(ctrl, ctrl_len);
            _cc->_w.append(data, data_len);
            return 0;
        }
        else
        {
            NEST_LOG(LOG_ERROR, "socket[%d] writev error; remote[%s], errno %d(%m) ", netfd, 
                _remote_addr.ToString(NULL, 0), errno);
            return -1;
        }
    }

    //  bytes >= 0
    ssize_t left_len  = 0;
    ssize_t cache_len = _cc->_w.data_len();
    if (bytes < cache_len)
    {
        _cc->_w.skip(bytes);
        _cc->_w.append(ctrl, ctrl_len);
        _cc->_w.append(data, data_len);
    }
    else if (bytes < (ssize_t)(cache_len + ctrl_len))
    {
        left_len = (cache_len + ctrl_len) - bytes;
        _cc->_w.skip(cache_len);
        _cc->_w.append(ctrl + ctrl_len - left_len, left_len);
        _cc->_w.append(data, data_len);
    }
    else if (bytes < (ssize_t)(cache_len + ctrl_len + data_len))
    {
        left_len = (cache_len + ctrl_len + data_len) - bytes;
        _cc->_w.skip(cache_len);
        _cc->_w.append(data + data_len - left_len, left_len);
    }
    else
    {
        _cc->_w.skip(cache_len);
    }

    // need write events
    if (_cc->_w.data_len() > 0)
    {
        EnableOutput();
        ApplyEvents();
    }
    else
    {
        DisableOutput();
        ApplyEvents();
    }

    return bytes;

}


/**
 * @brief ��������д�����������ݲ���
 * @return >=0 �ɹ�,���ط��͵��ֽ�; <0 ʧ��
 */
int32_t TNestChannel::SendCache()
{
    int32_t bytes = 0;

    if (_cc->_w.data_len() > 0) 
    {
        bytes = ::send(netfd, _cc->_w.data(), _cc->_w.data_len(), 0);
        if (bytes >= 0) 
        {
            _cc->_w.skip(bytes);
        } 
        else
        {   
        
            if ((errno != EAGAIN) && (errno != EINTR))
            {
                NEST_LOG(LOG_ERROR, "socket[%d] send error; remote[%s], errno %d(%m) ", netfd, 
                    _remote_addr.ToString(NULL, 0), errno);
                return -1;
            }
        }
    }

    // need write events
    if (_cc->_w.data_len() > 0)
    {
        EnableOutput();
        ApplyEvents();
    }
    else
    {
        DisableOutput();
        ApplyEvents();
    }

    return 0;
}


/**
 * @brief �ɶ��¼�������, �쳣�ر�����
 * @return >0 �ɹ�, <0 ʧ��, =0 Զ�˹ر�
 */
int32_t TNestChannel::InputNotify()
{
    if (netfd < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] error; remote[%s], may not init", netfd, 
            _remote_addr.ToString(NULL, 0));
        return -1;
    }
    
    int32_t ret = this->Recv();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] error or close; remote[%s], ret: %d", netfd, 
            _remote_addr.ToString(NULL, 0), ret);

        if (_dispatch)
        {
            _dispatch->DoError(this);
        } 
    }

    return ret;
}


/**
 * @brief ��д�¼�������, �쳣�ر�����
 * @return >=0 �ɹ�, <0 ʧ��
 */
int32_t TNestChannel::OutputNotify()
{
    if (netfd < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] error; remote[%s], may not init", netfd, 
            _remote_addr.ToString(NULL, 0));
        return -1;
    }

    // 1.���ӽ���״̬, �óɹ����
    if (_conn_state == CONNECTING)
    {
    	int error = 0;
    	socklen_t len = sizeof(error);
		if (::getsockopt(netfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error!=0) 
		{
	        NEST_LOG( LOG_ERROR, "socket[%d], remote[%s], connect failed (%m)", netfd, 
            						_remote_addr.ToString(NULL, 0));	
	        if (_dispatch)
	        {
	            _dispatch->DoError(this);
	        }
	        return -1;			
		}
    
        NEST_LOG(LOG_DEBUG, "socket[%d], remote[%s], connected OK", netfd, 
            _remote_addr.ToString(NULL, 0));
        _conn_state = CONNECTED;
        this->EnableInput();
        this->DisableOutput();
        this->ApplyEvents();

        NEST_LOG(LOG_TRACE, "socket connect ok, send regist, fd %d", netfd);
        if (_dispatch)
        {
            _dispatch->DoConnected(this);
        }

        return 0;
    }

    // 2. ����״̬, ���Է���cache����
    int32_t ret = this->SendCache();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "socket[%d] send error; remote[%s] log", netfd, 
            _remote_addr.ToString(NULL, 0));

        if (_dispatch)
        {
            _dispatch->DoError(this);
        } 
    }

    return ret;
    
}


/**
 * @brief �쳣����ע�ắ��
 */
int32_t TNestChannel::HangupNotify()
{
    if (_dispatch)
    {
        _dispatch->DoError(this);
    } 
    else 
    {
        NEST_LOG(LOG_ERROR, "socket[%d] no dispatch, error", netfd);
    }

    return 0;
}


/**
 * @brief ��ʼ��listen socket
 */
int32_t TNestListen::CreateSock()
{
    netfd = socket(_listen_addr.GetAddressDomain(), SOCK_STREAM, 0);
    if (netfd < 0)
    {
        NEST_LOG(LOG_ERROR, "create tcp socket failed, error: %d(%m)", errno);
        return -1;
    }
    
    int flags = 1;
    if (ioctl(netfd, FIONBIO, &flags) < 0)
    {
        NEST_LOG(LOG_ERROR, "set tcp socket unblock failed, error: %d(%m)", errno);
        close(netfd);
        netfd = -1;
        return -2;
    }

    int optval = 1;
    unsigned optlen = sizeof(optval);
    setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    if (bind(netfd, _listen_addr.GetAddress(), _listen_addr.GetAddressLen()) < 0)
    {
        close(netfd);
        netfd = -1;
        NEST_LOG(LOG_ERROR, "bind socket failed, error: %d(%m)", errno);
        return -3;
    }

    if (listen(netfd, 1024) < 0)
    {
        close(netfd);
        netfd = -1;
        NEST_LOG(LOG_ERROR, "listen socket failed, error: %d(%m)", errno);
        return -4;
    }

    return netfd;
}

/**
 * @brief �ɶ��¼�������, �����½���������Ϣ
 * @return >=0 �ɹ�, <0 ʧ��
 */
int32_t TNestListen::InputNotify()
{
    TNestAddrBuff clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    
    int32_t clt_fd = ::accept(netfd, (struct sockaddr*)&clt_addr, (socklen_t*)&clt_addr_len);
    if (clt_fd < 0)
    {
        if (errno == EAGAIN || errno == EINTR)
        {
            return 0;
        }
        else
        {
            NEST_LOG(LOG_ERROR, "Tcp accept error, fd %d, errno %d(%m)", netfd, errno);    
            return -1;
        }
    }

    int flags = 1;  
    if (::ioctl(clt_fd, FIONBIO, &flags) < 0)
    {
        NEST_LOG(LOG_ERROR, "Tcp client unblock error, fd %d, errno %d(%m)", clt_fd, errno);    
        close(clt_fd);
        return -2;
    }  

    int32_t ret = -1;
    TNestAddress  client_addr;
    client_addr.SetSockAddress(_listen_addr.GetAddressType(), (struct sockaddr*)&clt_addr, clt_addr_len);
    if (_dispatch) 
    {
        ret = _dispatch->DoAccept(clt_fd, &client_addr);
    }
    
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "Client %s accept mng failed, fd %d ", client_addr.ToString(NULL, 0), clt_fd);    
        close(clt_fd);
        return -3;
    }
    
    return 0;
}


/**
 * @brief ��д�¼�����, listen ����
 */
int32_t TNestListen::OutputNotify()
{
    NEST_LOG(LOG_ERROR, "Listen sock no write event attach, error, fd %d", netfd);    
    return 0;
}

/**
 * @brief �쳣֪ͨ����, Ŀǰ��ʱ����
 */
int32_t TNestListen::HangupNotify()
{
    NEST_LOG(LOG_ERROR, "Listen sock hangup, error, fd %d", netfd);    
    return 0;
}


/**
 * @brief ������ʼ������
 */
int32_t TNestListen::Init()
{
    // 1. create socket
    int32_t fd = this->CreateSock();
    if (fd < 0)
    {
        NEST_LOG(LOG_ERROR, "nest listen create error");    
        return -1;
    }

    // 2. event register
    this->EnableInput();
    this->DisableOutput();
    if (this->AttachPoller(TNestNetComm::Instance()->GetPollUnit()) < 0)
    {
        NEST_LOG(LOG_ERROR, "Nest listen event attach failed, fd %d(%m)", fd);    
        close(fd);
        return -2;
    }
    
    return 0;
}



/**
 * @brief ���캯������, Ĭ������buff
 */
TNestUdpListen::TNestUdpListen()
{
    _buff_size  = 64*1024;
    _msg_buff   = (char*)malloc(_buff_size);
    _tcp = false;
}

TNestUdpListen::~TNestUdpListen()
{
    if (_msg_buff) {
        free(_msg_buff);
        _msg_buff = NULL;
    }
    _buff_size = 0;
}



/**
 * @brief ��ʼ��listen socket
 */
int32_t TNestUdpListen::CreateSock()
{
    netfd = socket(_listen_addr.GetAddressDomain(), SOCK_DGRAM, 0);
    if (netfd < 0)
    {
        NEST_LOG(LOG_ERROR, "create datagrams socket failed, error: %d(%m)", errno);
        return -1;
    }
    
    int flags = 1;
    if (ioctl(netfd, FIONBIO, &flags) < 0)
    {
        NEST_LOG(LOG_ERROR, "set tcp socket unblock failed, error: %d(%m)", errno);
        close(netfd);
        netfd = -1;
        return -2;
    }

    if (bind(netfd, _listen_addr.GetAddress(), _listen_addr.GetAddressLen()) < 0)
    {
        close(netfd);
        netfd = -1;
        NEST_LOG(LOG_ERROR, "bind socket failed, error: %d(%m)", errno);
        return -3;
    }

    return netfd;
}

/**
 * @brief ����ɶ��¼�, ֱ�Ӵ�����
 */
int32_t TNestUdpListen::InputNotify()
{
    TNestAddrBuff from_addr;
    socklen_t addr_len = sizeof(from_addr);

    size_t bytes = ::recvfrom(netfd, _msg_buff, _buff_size, 0, 
                           (struct sockaddr *)&from_addr, &addr_len);
    if (bytes > 0)
    {
        TNestBlob  blob;
        blob._type = NEST_BLOB_TYPE_DGRAM;
        blob._buff = _msg_buff;
        blob._len  = bytes;
        blob._recv_fd = netfd;
        blob._remote_addr.SetSockAddress(_listen_addr.GetAddressType(),
                          (struct sockaddr*)&from_addr, addr_len);
        if (_dispatch)
        {
            return _dispatch->DoRecv(&blob);
        } 
        else 
        {
            NEST_LOG(LOG_ERROR, "socket[%d] no dispatch, error", netfd);
            return -100;
        }
    }

    return 0;

}


/**
 * @brief sessionȫ�ֹ�����
 * @return ȫ�־��ָ��
 */
TNestNetComm* TNestNetComm::_instance = NULL;
TNestNetComm* TNestNetComm::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TNestNetComm();
    }

    return _instance;
}

/**
 * @brief session����ȫ�ֵ����ٽӿ�
 */
void TNestNetComm::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief ���罻�����ӹ���Ĺ��캯��
 */
TNestNetComm::TNestNetComm()
{
    _poll_unit   = NULL;
    _mem_pool    = new CMemPool();
}

/**
 * @brief ��������
 */
TNestNetComm::~TNestNetComm()
{

    if (_mem_pool) {
        delete _mem_pool;
        _mem_pool = NULL;
    }
}

/**
 * @brief ����һ��poll unit����
 */
CPollerUnit* TNestNetComm::CreatePollUnit()
{
    CPollerUnit* poll_unit = new CPollerUnit(10240);
    if (poll_unit) {
        poll_unit->InitializePollerUnit();
    }

    return poll_unit;
}

/**
 * @brief ����һ��poll unit����
 */
void TNestNetComm::DestroyPollUnit(CPollerUnit* unit)
{
    if (unit) {
        delete unit;
    }
}

/**
 * @brief ִ��ȫ�ֵ�poll����
 */
void TNestNetComm::RunPollUnit(uint32_t wait_time)
{
    if (_poll_unit) {
        _poll_unit->WaitPollerEvents(wait_time);
        _poll_unit->ProcessPollerEvents();
    }
}



