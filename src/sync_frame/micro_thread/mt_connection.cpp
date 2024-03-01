/**
 *  @file mt_connection.cpp
 *  @info ΢�߳���Ϣ�������ӹ���ʵ��
 *  @time 20130924
 **/
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "micro_thread.h"
#include "mt_msg.h"
#include "mt_notify.h"
#include "mt_connection.h"
#include "mt_sys_hook.h"

using namespace std;
using namespace NS_MICRO_THREAD;


/**
 * @brief  ΢�߳����ӻ��๹��������
 */
IMtConnection::IMtConnection() 
{
    _type       = OBJ_CONN_UNDEF;
    _action     = NULL;
    _ntfy_obj   = NULL;
    _msg_buff   = NULL;
}
IMtConnection::~IMtConnection() 
{
    if (_ntfy_obj) {
        NtfyObjMgr::Instance()->FreeNtfyObj(_ntfy_obj);
        _ntfy_obj = NULL;
    }

    if (_msg_buff) {
        MsgBuffPool::Instance()->FreeMsgBuf(_msg_buff);
        _msg_buff = NULL;
    }
}


/**
 * @brief ���ӻ��ո����������
 */
void IMtConnection::Reset()
{
    if (_ntfy_obj) {
        NtfyObjMgr::Instance()->FreeNtfyObj(_ntfy_obj);
        _ntfy_obj = NULL;
    }

    if (_msg_buff) {
        MsgBuffPool::Instance()->FreeMsgBuf(_msg_buff);
        _msg_buff = NULL;
    }

    _action     = NULL;
    _ntfy_obj   = NULL;
    _msg_buff   = NULL;
}


/**
 * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
 * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
 */
int UdpShortConn::CreateSocket()
{
    // 1. UDP������, ÿ���´�SOCKET
    _osfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_osfd < 0)
    {
        MTLOG_ERROR("socket create failed, errno %d(%s)", errno, strerror(errno));
        return -1;
    }
    
    // 2. ����������
    int flags = 1;
    if (ioctl(_osfd, FIONBIO, &flags) < 0)
    {
        MTLOG_ERROR("socket unblock failed, errno %d(%s)", errno, strerror(errno));
        close(_osfd);
        _osfd = -1;
        return -2;
    }

    // 3. ���¹�����Ϣ
    if (_ntfy_obj) {
        _ntfy_obj->SetOsfd(_osfd);
    }

    return _osfd;
}

/**
 * @brief �ر�ϵͳsocket, ����һЩ״̬
 */
int UdpShortConn::CloseSocket()
{
    if (_osfd < 0) 
    {
        return 0;
    }

    close(_osfd);
    _osfd = -1;
    
    return 0;
}


/**
 * @brief ���Է�������, ������һ��
 * @return 0 ���ͱ��ж�, ������. <0 ϵͳʧ��. >0 ���η��ͳɹ�
 */
int UdpShortConn::SendData()
{
    if (!_action || !_msg_buff) {
        MTLOG_ERROR("conn not set action %p, or msg %p, error", _action, _msg_buff);
        return -100;
    }

    mt_hook_syscall(sendto);
    int ret = mt_real_func(sendto)(_osfd, _msg_buff->GetMsgBuff(), _msg_buff->GetMsgLen(), 0, 
                (struct sockaddr*)_action->GetMsgDstAddr(), sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
        {
            return 0;
        }
        else
        {
            MTLOG_ERROR("socket send failed, fd %d, errno %d(%s)", _osfd, 
                      errno, strerror(errno));
            return -2;
        }
    }
    else
    {
        _msg_buff->SetHaveSndLen(ret);
        return ret;
    }
}

/**
 * @brief ���Խ�������, ������һ��
 * @param buff ���ջ�����ָ��
 * @return -1 �Զ˹ر�. -2 ϵͳʧ��. >0 ���ν��ճɹ�
 */
int UdpShortConn::RecvData()
{
    if (!_action || !_msg_buff) {
        MTLOG_ERROR("conn not set action %p, or msg %p, error", _action, _msg_buff);
        return -100;
    }

    struct sockaddr_in  from;
    socklen_t fromlen = sizeof(from);
    mt_hook_syscall(recvfrom);
    int ret = mt_real_func(recvfrom)(_osfd, _msg_buff->GetMsgBuff(), _msg_buff->GetMaxLen(),
                       0, (struct sockaddr*)&from, &fromlen);
    if (ret < 0)
    {
        if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
        {
            return 0;
        }
        else
        {
            MTLOG_ERROR("socket recv failed, fd %d, errno %d(%s)", _osfd, 
                      errno, strerror(errno));
            return -2;  // ϵͳ����
        }
    }
    else if (ret == 0)
    {
        return -1;  // �Զ˹ر�
    }
    else
    {
        _msg_buff->SetHaveRcvLen(ret);
    }
    
    // �����ļ��, >0 �հ�����; =0 �����ȴ�; <0(-65535����)�����쳣  
    ret = _action->DoInput();
    if (ret > 0)
    {
        _msg_buff->SetMsgLen(ret);
        return ret;
    }
    else if (ret == 0)
    {
        return 0;
    }
    else if (ret == -65535)
    {
        _msg_buff->SetHaveRcvLen(0);
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief ���ӻ��ո����������
 */
void UdpShortConn::Reset()
{
    CloseSocket();
    this->IMtConnection::Reset();
}


/**
 * @brief  ���Ӵ���Զ�˻Ựͨ��, ��TCP��connect��
 * @return 0 -�ɹ�, < 0 ʧ�� 
 */
int TcpKeepConn::OpenCnnect()
{
    if (!_action || !_msg_buff) {
        MTLOG_ERROR("conn not set action %p, or msg %p, error", _action, _msg_buff);
        return -100;
    }

    int err = 0;
    mt_hook_syscall(connect);
    int ret = mt_real_func(connect)(_osfd, (struct sockaddr*)_action->GetMsgDstAddr(), sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        err = errno;
        if (err == EISCONN)
        {
            return 0;
        }
        else
        {
            if ((err == EINPROGRESS) || (err == EALREADY) || (err == EINTR))
            {
                MTLOG_DEBUG("Open connect not ok, maybe first try, sock %d, errno %d", _osfd, err);
                return -1;
            }
            else
            {
                MTLOG_ERROR("Open connect not ok, sock %d, errno %d", _osfd, err);
                return -2;
            }
        }
    }
    else
    {
        return 0;
    }
}

/**
 * @brief ����sock��TCP��������
 */
int TcpKeepConn::CreateSocket()
{
    if (_osfd > 0)        // ��������ʱ, ��������������; ������������ntfyfd
    {
        if (_ntfy_obj) {
            _ntfy_obj->SetOsfd(_osfd);
        }
        
        return _osfd;
    }

    // ��һ�ν���ʱ, ����socket
    _osfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_osfd < 0)
    {
        MTLOG_ERROR("create tcp socket failed, error: %d", errno);
        return -1;
    }
    
    // ����������
    int flags = 1;
    if (ioctl(_osfd, FIONBIO, &flags) < 0)
    {
        MTLOG_ERROR("set tcp socket unblock failed, error: %d", errno);
        close(_osfd);
        _osfd = -1;
        return -2;
    }

    // ���¹�����Ϣ
    _keep_ntfy.SetOsfd(_osfd);
    _keep_ntfy.DisableOutput();
    _keep_ntfy.EnableInput(); 
    
    if (_ntfy_obj) {
        _ntfy_obj->SetOsfd(_osfd);
    }

    return _osfd;
}

/**
 * @brief ���Է�������, ������һ��
 * @param dst  ����Ŀ�ĵ�ַ
 * @param buff ���ͻ�����ָ��
 * @param size �����͵���󳤶�
 * @return 0 ���ͱ��ж�, ������. <0 ϵͳʧ��. >0 ���η��ͳɹ�
 */
int TcpKeepConn::SendData()
{
    if (!_action || !_msg_buff) {
        MTLOG_ERROR("conn not set action %p, or msg %p, error", _action, _msg_buff);
        return -100;
    }

    char* msg_ptr = (char*)_msg_buff->GetMsgBuff();
    int msg_len = _msg_buff->GetMsgLen();
    int have_send_len = _msg_buff->GetHaveSndLen();
    mt_hook_syscall(send);
    int ret = mt_real_func(send)(_osfd, msg_ptr + have_send_len, msg_len - have_send_len, 0);
    if (ret == -1)
    {
        if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
        {
            return 0;
        }
        else
        {
            MTLOG_ERROR("send tcp socket failed, error: %d", errno);
            return -1;
        }
    }
    else
    {
        have_send_len += ret;
        _msg_buff->SetHaveSndLen(have_send_len);
    }

    // ȫ���������, ���سɹ�, ��������ȴ�
    if (have_send_len >= msg_len)
    {
        return msg_len;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief ���Խ�������, ������һ��
 * @param buff ���ջ�����ָ��
 * @return -1 �Զ˹ر�. -2 ϵͳʧ��. >0 ���ν��ճɹ�
 */
int TcpKeepConn::RecvData()
{
    if (!_action || !_msg_buff) {
        MTLOG_ERROR("conn not set action %p, or msg %p, error", _action, _msg_buff);
        return -100;
    }

    char* msg_ptr = (char*)_msg_buff->GetMsgBuff();
    int max_len = _msg_buff->GetMaxLen();
    int have_rcv_len = _msg_buff->GetHaveRcvLen();
    mt_hook_syscall(recv);
    int ret = mt_real_func(recv)(_osfd, (char*)msg_ptr + have_rcv_len, max_len - have_rcv_len, 0); 
    if (ret < 0)
    {
        if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
        {
            return 0;
        }
        else
        {
            MTLOG_ERROR("recv tcp socket failed, error: %d", errno);
            return -2;  // ϵͳ����
        }
    }
    else if (ret == 0)
    {
        MTLOG_ERROR("tcp remote close, address: %s[%d]", 
                inet_ntoa(_dst_addr.sin_addr), ntohs(_dst_addr.sin_port));
        return -1;  // �Զ˹ر�
    }
    else
    {
        have_rcv_len += ret;
        _msg_buff->SetHaveRcvLen(have_rcv_len);
    }

    // �����ļ��, >0 �հ�����; =0 �����ȴ�; <0(-65535����)�����쳣  
    ret = _action->DoInput();
    if (ret > 0)
    {
        _msg_buff->SetMsgLen(have_rcv_len);
        return ret;
    }
    else if (ret == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief �ر�ϵͳsocket, ����һЩ״̬
 */
int TcpKeepConn::CloseSocket()
{
    if (_osfd < 0) 
    {
        return 0;
    }
    _keep_ntfy.SetOsfd(-1);
    
    close(_osfd);
    _osfd = -1;

    return 0;
}

/**
 * @brief ���ӻ��ո����������
 */
void TcpKeepConn::Reset()
{
    memset(&_dst_addr, 0 ,sizeof(_dst_addr));
    CloseSocket();
    this->IMtConnection::Reset();
}

/**
 * @brief ���ӻ��ո����������
 */
void TcpKeepConn::ConnReuseClean()
{
    this->IMtConnection::Reset();
}

/**
 * @brief Idle���洦��, epoll ����Զ�˹رյ�
 */
bool TcpKeepConn::IdleAttach()
{
    if (_osfd < 0) {
        MTLOG_ERROR("obj %p attach failed, fd %d error", this, _osfd);
        return false;
    }

    if (_keep_flag & TCP_KEEP_IN_EPOLL) {
        MTLOG_ERROR("obj %p repeat attach, error", this);
        return true;
    }

    _keep_ntfy.DisableOutput();
    _keep_ntfy.EnableInput();

    // ���ʱ�����
    CTimerMng* timer = MtFrame::Instance()->GetTimerMng();
    if ((NULL == timer) || !timer->start_timer(this, _keep_time))
    {
        MTLOG_ERROR("obj %p attach timer failed, error", this);
        return false;
    }

    if (MtFrame::Instance()->EpollAddObj(&_keep_ntfy))
    {
        _keep_flag |= TCP_KEEP_IN_EPOLL;
        return true;
    }
    else
    {
        MTLOG_ERROR("obj %p attach failed, error", this);
        return false;
    }    
}

/**
 * @brief Idleȡ�����洦��, �����ɿ����߳�����Զ�˹ر�
 */
bool TcpKeepConn::IdleDetach()
{
    if (_osfd < 0) {
        MTLOG_ERROR("obj %p detach failed, fd %d error", this, _osfd);
        return false;
    }

    if (!(_keep_flag & TCP_KEEP_IN_EPOLL)) {
        MTLOG_DEBUG("obj %p repeat detach, error", this);
        return true;
    }

    _keep_ntfy.DisableOutput();
    _keep_ntfy.EnableInput();

    // ���ʱ��ɾ��
    CTimerMng* timer = MtFrame::Instance()->GetTimerMng();
    if (NULL != timer) 
    {
        timer->stop_timer(this);
    }

    if (MtFrame::Instance()->EpollDelObj(&_keep_ntfy))
    {
        _keep_flag &= ~TCP_KEEP_IN_EPOLL;
        return true;
    }
    else
    {
        MTLOG_ERROR("obj %p detach failed, error", this);
        return false;
    }    
}


/**
 * @brief ��ʱ֪ͨ����, ����ʵ���߼�
 */
void TcpKeepConn::timer_notify()
{
    MTLOG_DEBUG("keep timeout[%u], fd %d, close connection", _keep_time, _osfd);
    ConnectionMgr::Instance()->CloseIdleTcpKeep(this);
}

/**
 * @brief ��������������
 */
TcpKeepMgr::TcpKeepMgr() 
{
    _keep_hash = new HashList(10000);
}

TcpKeepMgr::~TcpKeepMgr() 
{
    if (!_keep_hash) {
        return;
    }
    
    HashKey* hash_item = _keep_hash->HashGetFirst();
    while (hash_item)
    {
        delete hash_item;
        hash_item = _keep_hash->HashGetFirst();
    }
    
    delete _keep_hash;
    _keep_hash = NULL;
}


/**
 * @brief ��IP��ַ��ȡTCP�ı�������
 */
TcpKeepConn* TcpKeepMgr::GetTcpKeepConn(struct sockaddr_in* dst)
{
    TcpKeepConn* conn = NULL;
    if (NULL == dst) 
    {
        MTLOG_ERROR("input param dst null, error");
        return NULL;
    }
    
    TcpKeepKey key(dst);
    TcpKeepKey* conn_list = (TcpKeepKey*)_keep_hash->HashFindData(&key);
    if ((NULL == conn_list) || (NULL == conn_list->GetFirstConn()))
    {
        conn = _mem_queue.AllocPtr();
        if (conn) {
            conn->SetDestAddr(dst);
        }
    }
    else
    {
        conn = conn_list->GetFirstConn();
        conn_list->RemoveConn(conn);
        conn->IdleDetach();
    }

    return conn;
}

/**
 * @brief ��IP��ַ����TCP�ı�������
 */
bool TcpKeepMgr::RemoveTcpKeepConn(TcpKeepConn* conn) 
{
    struct sockaddr_in* dst = conn->GetDestAddr();
    if ((dst->sin_addr.s_addr == 0) || (dst->sin_port == 0))
    {
        MTLOG_ERROR("sock addr, invalid, %x:%d", dst->sin_addr.s_addr, dst->sin_port);
        return false;
    }
    
    TcpKeepKey key(dst);
    TcpKeepKey* conn_list = (TcpKeepKey*)_keep_hash->HashFindData(&key);
    if (!conn_list) 
    {
        MTLOG_ERROR("no conn cache list, invalid, %x:%d", dst->sin_addr.s_addr, dst->sin_port);
        return false;
    }
    
    conn->IdleDetach();
    conn_list->RemoveConn(conn);

    return true;
    
}

    
/**
 * @brief ��IP��ַ����TCP�ı�������
 */
bool TcpKeepMgr::CacheTcpKeepConn(TcpKeepConn* conn) 
{
    struct sockaddr_in* dst = conn->GetDestAddr();
    if ((dst->sin_addr.s_addr == 0) || (dst->sin_port == 0))
    {
        MTLOG_ERROR("sock addr, invalid, %x:%d", dst->sin_addr.s_addr, dst->sin_port);
        return false;
    }
    
    TcpKeepKey key(dst);
    TcpKeepKey* conn_list = (TcpKeepKey*)_keep_hash->HashFindData(&key);
    if (!conn_list) 
    {
        conn_list = new TcpKeepKey(conn->GetDestAddr());
        if (!conn_list) {
            MTLOG_ERROR("new conn list failed, error");
            return false;
        }
        _keep_hash->HashInsert(conn_list);
    }

    if (!conn->IdleAttach()) 
    {
        MTLOG_ERROR("conn IdleAttach failed, error");
        return false;
    }
    
    conn->ConnReuseClean();         
    conn_list->InsertConn(conn);
    

    return true;
    
}

/**
 * @brief �رջ򻺴�tcp������
 */
void TcpKeepMgr::FreeTcpKeepConn(TcpKeepConn* conn, bool force_free)
{
    if (force_free) 
    {
        conn->Reset();
        _mem_queue.FreePtr(conn);
        return;
    }
    else
    {
        if (!CacheTcpKeepConn(conn))
        {
            conn->Reset();
            _mem_queue.FreePtr(conn);
            return;
        }
    }
}
    


/**
 * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
 * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
 */
int UdpSessionConn::CreateSocket()
{
    // 1. session������, ��֪ͨ���󴴽�����fd
    if (!_action || !_ntfy_obj) {
        MTLOG_ERROR("conn not set action %p, or _ntfy_obj %p, error", _action, _ntfy_obj);
        return -100;
    }
    SessionProxy* proxy = dynamic_cast<SessionProxy*>(_ntfy_obj);
    if (!proxy) {
        MTLOG_ERROR("ntfy obj not match, _ntfy_obj %p, error", _ntfy_obj);
        return -200;
    }    
    ISessionNtfy* real_ntfy = proxy->GetRealNtfyObj();
    if (!real_ntfy) {
        MTLOG_ERROR("real ntfy obj not match, _ntfy_obj %p, error", _ntfy_obj);
        return -300;
    }

    // 2. ί�ɴ���, ���¾����Ϣ
    int osfd = real_ntfy->GetOsfd();
    if (osfd <= 0)
    {
        osfd = real_ntfy->CreateSocket();
        if (osfd <= 0) {
            MTLOG_ERROR("real ntfy obj create fd failed, _ntfy_obj %p, error", real_ntfy);
            return -400;
        }
    }
    _ntfy_obj->SetOsfd(osfd);
    
    return osfd;
}

/**
 * @brief �ر�ϵͳsocket, ����һЩ״̬
 */
int UdpSessionConn::CloseSocket()
{
    return 0;
}


/**
 * @brief ���Է�������, ������һ��
 * @return 0 ���ͱ��ж�, ������. <0 ϵͳʧ��. >0 ���η��ͳɹ�
 */
int UdpSessionConn::SendData()
{
    if (!_action || !_msg_buff || !_ntfy_obj) {
        MTLOG_ERROR("conn not set action %p, or msg %p, ntfy %p error", _action, _msg_buff, _ntfy_obj);
        return -100;
    }

    mt_hook_syscall(sendto);
    int ret = mt_real_func(sendto)(_ntfy_obj->GetOsfd(), _msg_buff->GetMsgBuff(), _msg_buff->GetMsgLen(), 0, 
                (struct sockaddr*)_action->GetMsgDstAddr(), sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
        {
            return 0;
        }
        else
        {
            MTLOG_ERROR("socket send failed, fd %d, errno %d(%s)", _ntfy_obj->GetOsfd(), 
                      errno, strerror(errno));
            return -2;
        }
    }
    else
    {
        _msg_buff->SetHaveSndLen(ret);
        return ret;
    }
}

/**
 * @brief ���Խ�������, ������һ��
 * @param buff ���ջ�����ָ��
 * @return 0 -�����ȴ�����; >0 ���ճɹ�; < 0 ʧ��
 */
int UdpSessionConn::RecvData()
{
    if (!_ntfy_obj || !_msg_buff) {
        MTLOG_ERROR("conn not set _ntfy_obj %p, or msg %p, error", _ntfy_obj, _msg_buff);
        return -100;
    }

    if (_ntfy_obj->GetRcvEvents() <= 0) {
        MTLOG_DEBUG("conn _ntfy_obj %p, no recv event, retry it", _ntfy_obj);
        return 0;
    }

    //  UDP Session ֪ͨ���滻msg buff, ͨ��type�ж�
    int msg_len = _msg_buff->GetMsgLen();
    if (BUFF_RECV == _msg_buff->GetBuffType())
    {
        return msg_len;
    }
    else
    {
        MTLOG_DEBUG("conn msg buff %p, no recv comm", _msg_buff);
        return 0;
    }
}


/**
 * @brief sessionȫ�ֹ�����
 * @return ȫ�־��ָ��
 */
ConnectionMgr* ConnectionMgr::_instance = NULL;
ConnectionMgr* ConnectionMgr::Instance (void)
{
    if (NULL == _instance)
    {
        _instance = new ConnectionMgr();
    }

    return _instance;
}

/**
 * @brief session����ȫ�ֵ����ٽӿ�
 */
void ConnectionMgr::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief ��Ϣbuff�Ĺ��캯��
 */
ConnectionMgr::ConnectionMgr()
{
}

/**
 * @brief ��������, ��������Դ, ������������
 */
ConnectionMgr::~ConnectionMgr()
{
}

/**
 * @brief ��ȡ�ӿ�
 */
IMtConnection* ConnectionMgr::GetConnection(CONN_OBJ_TYPE type, struct sockaddr_in* dst)
{
    switch (type)
    {
        case OBJ_SHORT_CONN:
            return _udp_short_queue.AllocPtr();
            break;

        case OBJ_TCP_KEEP:
            return _tcp_keep_mgr.GetTcpKeepConn(dst);
            break;

        case OBJ_UDP_SESSION:
            return _udp_session_queue.AllocPtr();
            break;

        default:
            return NULL;
            break;
    }

}

/**
 * @brief ���սӿ�
 */
void ConnectionMgr::FreeConnection(IMtConnection* conn, bool force_free)
{
    if (!conn) {
        return;
    }
    CONN_OBJ_TYPE type = conn->GetConnType();
 
    switch (type)
    {
        case OBJ_SHORT_CONN:
            conn->Reset();
            return _udp_short_queue.FreePtr(dynamic_cast<UdpShortConn*>(conn));
            break;

        case OBJ_TCP_KEEP:
            return _tcp_keep_mgr.FreeTcpKeepConn(dynamic_cast<TcpKeepConn*>(conn), force_free);
            break;

        case OBJ_UDP_SESSION:
            conn->Reset();
            return _udp_session_queue.FreePtr(dynamic_cast<UdpSessionConn*>(conn));
            break;

        default:
            break;
    }

    delete conn;
    return;
}


/**
 * @brief �ر�idle��tcp������
 */
void ConnectionMgr::CloseIdleTcpKeep(TcpKeepConn* conn)
{
    _tcp_keep_mgr.RemoveTcpKeepConn(conn);
    _tcp_keep_mgr.FreeTcpKeepConn(conn, true);
}


