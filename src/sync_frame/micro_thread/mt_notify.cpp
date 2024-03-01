/**
 *  @file mt_notify.cpp
 *  @info ΢�̵߳���ע��������ʵ��
 *  @time 20130924
 **/
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "micro_thread.h"
#include "mt_session.h"
#include "mt_msg.h"
#include "mt_notify.h"
#include "mt_connection.h"
#include "mt_sys_hook.h"

using namespace std;
using namespace NS_MICRO_THREAD;


/**
 * @brief ֪ͨ�������ȴ�״̬, ����ȴ�������
 * @param proxy �����sessionģ��
 */
void ISessionNtfy::InsertWriteWait(SessionProxy* proxy) 
{
    if (!proxy->_flag) {
        TAILQ_INSERT_TAIL(&_write_list, proxy, _write_entry);
        proxy->_flag = 1;
    }    
}

/**
 * @brief ֪ͨ�����Ƴ��ȴ�״̬
 * @param proxy �����sessionģ��
 */
void ISessionNtfy::RemoveWriteWait(SessionProxy* proxy) 
{
    if (proxy->_flag) {
        TAILQ_REMOVE(&_write_list, proxy, _write_entry);
        proxy->_flag = 0;
    }    
}

/**
 * @brief �۲���ģʽ, ֪ͨд�ȴ��߳�
 * @info UDP����֪ͨÿ���߳�ִ��д����, TCP��Ҫ�Ŷ�д
 */
void UdpSessionNtfy::NotifyWriteWait()
{
    MtFrame* frame = MtFrame::Instance();
    SessionProxy* proxy = NULL;
    MicroThread* thread = NULL;
    TAILQ_FOREACH(proxy, &_write_list, _write_entry)
    {
        proxy->SetRcvEvents(EPOLLOUT);
        
        thread = proxy->GetOwnerThread();
        if (thread && thread->HasFlag(MicroThread::IO_LIST))
        {
            frame->RemoveIoWait(thread);
            frame->InsertRunable(thread);
        }
    }
}

/**
 *  @brief ����socket, �����ɶ��¼�
 *  @return fd�ľ��, <0 ʧ��
 */
int UdpSessionNtfy::CreateSocket()
{
    // 1. UDP������, ÿ���´�SOCKET
    int osfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (osfd < 0)
    {
        MTLOG_ERROR("socket create failed, errno %d(%s)", errno, strerror(errno));
        return -1;
    }
    
    // 2. ����������
    int flags = 1;
    if (ioctl(osfd, FIONBIO, &flags) < 0)
    {
        MTLOG_ERROR("socket unblock failed, errno %d(%s)", errno, strerror(errno));
        close(osfd);
        osfd = -1;
        return -2;
    }

    // ��ѡbindִ��, ���ñ���port��ִ��
    if (_local_addr.sin_port != 0)
    {
        int ret = bind(osfd, (struct sockaddr *)&_local_addr, sizeof(_local_addr));
        if (ret < 0)
        {
            MTLOG_ERROR("socket bind(%s:%d) failed, errno %d(%s)",  inet_ntoa(_local_addr.sin_addr), 
                    ntohs(_local_addr.sin_port), errno, strerror(errno));
            close(osfd);
            osfd = -1;
            return -3;
        }
    }

    // 3. ���¹�����Ϣ, Ĭ��udp session ���� epollin
    this->SetOsfd(osfd);
    this->EnableInput();
    MtFrame* frame = MtFrame::Instance();
    frame->EpollNtfyReg(osfd, this);
    frame->EpollCtrlAdd(osfd, EPOLLIN);
    
    return osfd;
}


/**
 *  @brief �ر�socket, ֹͣ�����ɶ��¼�
 */
void UdpSessionNtfy::CloseSocket()
{
    int osfd = this->GetOsfd();
    if (osfd > 0)
    {
        MtFrame* frame = MtFrame::Instance();
        frame->EpollCtrlDel(osfd, EPOLLIN);
        frame->EpollNtfyReg(osfd, NULL);
        this->DisableInput();
        this->SetOsfd(-1);
        close(osfd);
    }
}


/**
 *  @brief �ɶ��¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int UdpSessionNtfy::InputNotify()
{
    while (1)
    {
        int ret = 0;
        int have_rcv_len = 0;

        // 1. ��ȡ�հ�������, ����ѡ��δ�����������buff
        if (!_msg_buff) {
            _msg_buff = MsgBuffPool::Instance()->GetMsgBuf(this->GetMsgBuffSize());
            if (NULL == _msg_buff) {
                MTLOG_ERROR("Get memory failed, size %d, wait next time", this->GetMsgBuffSize());
                return 0;
            }
            _msg_buff->SetBuffType(BUFF_RECV);
        }
        char* buff = (char*)_msg_buff->GetMsgBuff();

        // 2. ��ȡsocket, �հ�����
        int osfd = this->GetOsfd();
        struct sockaddr_in  from;
        socklen_t fromlen = sizeof(from); 
        mt_hook_syscall(recvfrom);
        ret = mt_real_func(recvfrom)(osfd, buff, _msg_buff->GetMaxLen(),
                       0, (struct sockaddr*)&from, &fromlen);
        if (ret < 0)
        {
            if ((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
            {
                return 0;
            }
            else
            {
                MTLOG_ERROR("recv error, fd %d", osfd);
                return 0;  // ϵͳ����, UDP �ݲ��ر�
            }
        }
        else if (ret == 0)
        {
            MTLOG_DEBUG("remote close connection, fd %d", osfd);
            return 0;  // �Զ˹ر�, UDP �ݲ��ر�
        }
        else
        {
            have_rcv_len = ret;
            _msg_buff->SetHaveRcvLen(have_rcv_len);
            _msg_buff->SetMsgLen(have_rcv_len);
        }

        // 3. �����Ϣ��������, ��ȡsessionid
        int sessionid = 0;
        ret = this->GetSessionId(buff, have_rcv_len, sessionid);
        if (ret <= 0)
        {
            MTLOG_ERROR("recv get session failed, len %d, fd %d, drop it", 
                       have_rcv_len, osfd);
            MsgBuffPool::Instance()->FreeMsgBuf(_msg_buff);
            _msg_buff = NULL;
            return 0;
        }

        // 4. ӳ���ѯthread���, ����handle���, ���ö��¼�����, �ҽ�msgbuff
        ISession* session = SessionMgr::Instance()->FindSession(sessionid);
        if (NULL == session) 
        {
            MT_ATTR_API(350403, 1); // session �����ѳ�ʱ
            MTLOG_DEBUG("session %d, not find, maybe timeout, drop pkg", sessionid);
            MsgBuffPool::Instance()->FreeMsgBuf(_msg_buff);
            _msg_buff = NULL;
            return 0;
        }

        // 5. �ҽ�recvbuff, �����߳�
        IMtConnection* conn = session->GetSessionConn();
        MicroThread* thread = session->GetOwnerThread();
        if (!thread || !conn || !conn->GetNtfyObj()) 
        {
            MTLOG_ERROR("sesson obj %p, no thread ptr %p, no conn %p wrong",
                    session, thread, conn);
            MsgBuffPool::Instance()->FreeMsgBuf(_msg_buff);
            _msg_buff = NULL;
            return 0;
        }
        MtMsgBuf* msg = conn->GetMtMsgBuff();
        if (msg) {
            MsgBuffPool::Instance()->FreeMsgBuf(msg);
        }
        conn->SetMtMsgBuff(_msg_buff);
        _msg_buff = NULL;

        conn->GetNtfyObj()->SetRcvEvents(EPOLLIN);
        if (thread->HasFlag(MicroThread::IO_LIST))
        {
            MtFrame* frame = MtFrame::Instance();
            frame->RemoveIoWait(thread);
            frame->InsertRunable(thread);
        }
    }

    return 0;
}

/**
 *  @brief ��д�¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int UdpSessionNtfy::OutputNotify()
{
    NotifyWriteWait();
    return 0;
}

/**
 *  @brief �쳣֪ͨ�ӿ�, �ر�fd����, thread�ȴ�����ʱ
 *  @return ���Է���ֵ, ���������¼�����
 */
int UdpSessionNtfy::HangupNotify()
{
    // 1. ����epoll ctl�����¼�
    MtFrame* frame = MtFrame::Instance();
    frame->EpollCtrlDel(this->GetOsfd(), this->GetEvents());

    MTLOG_ERROR("sesson obj %p, recv error event. fd %d", this, this->GetOsfd());
    
    // 2. ���´�socket
    CloseSocket();

    // 3. �ؼ���epoll listen
    CreateSocket();

    return 0;
}

/**
 *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
 *  @param args fd���ö����ָ��
 *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
 *  @info  Ĭ���Ǽ����ɶ��¼���, ����ֻ�����д�¼��ļ���ɾ��
 */
int UdpSessionNtfy::EpollCtlAdd(void* args)
{
    MtFrame* frame = MtFrame::Instance();
    FdRef* fd_ref = (FdRef*)args;
    //ASSERT(fd_ref != NULL);

    int osfd = this->GetOsfd();

    // ֪ͨ������Ҫ����, FD֪ͨ���������ϲ��Ḵ��, ��������ͻ���, �쳣log��¼
    EpollerObj* old_obj = fd_ref->GetNotifyObj();
    if ((old_obj != NULL) && (old_obj != this))
    {
        MTLOG_ERROR("epfd ref conflict, fd: %d, old: %p, now: %p", osfd, old_obj, this);
        return -1;
    }

    // ���ÿ�ܵ�epoll ctl�ӿ�, ����epoll ctrlϸ��
    if (!frame->EpollCtrlAdd(osfd, EPOLLOUT))
    {
        MTLOG_ERROR("epfd ref add failed, log");
        return -2;
    }
    this->EnableOutput();
    
    return 0;
}

/**
 *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
 *  @param args fd���ö����ָ��
 *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
 */
int UdpSessionNtfy::EpollCtlDel(void* args)
{
    MtFrame* frame = MtFrame::Instance();
    FdRef* fd_ref = (FdRef*)args;
    //ASSERT(fd_ref != NULL);

    int osfd = this->GetOsfd();
    
    // ֪ͨ������Ҫ����, FD֪ͨ���������ϲ��Ḵ��, ��������ͻ���, �쳣log��¼
    EpollerObj* old_obj = fd_ref->GetNotifyObj();
    if (old_obj != this)
    {
        MTLOG_ERROR("epfd ref conflict, fd: %d, old: %p, now: %p", osfd, old_obj, this);
        return -1;
    }

    // ���ÿ�ܵ�epoll ctl�ӿ�, ����epoll ctrlϸ��
    if (!frame->EpollCtrlDel(osfd, EPOLLOUT))
    {
        MTLOG_ERROR("epfd ref del failed, log");
        return -2;
    }
    this->DisableOutput();

    return 0;

}


/**
 *  @brief �ɶ��¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int TcpKeepNtfy::InputNotify()
{
    KeepaliveClose();
    return -1;
}
    
/**
 *  @brief ��д�¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int TcpKeepNtfy::OutputNotify()
{
    KeepaliveClose();
    return -1;
}
    
/**
 *  @brief �쳣֪ͨ�ӿ�
 *  @return ���Է���ֵ, ���������¼�����
 */
int TcpKeepNtfy::HangupNotify()
{
    KeepaliveClose();
    return -1;
}

    
/**
 *  @brief ����ʵ�����ӹرղ���
 */
void TcpKeepNtfy::KeepaliveClose()
{
    if (_keep_conn) {
        MTLOG_DEBUG("remote close, fd %d, close connection", _fd);
        ConnectionMgr::Instance()->CloseIdleTcpKeep(_keep_conn);
    } else {
        MTLOG_ERROR("_keep_conn ptr null, error");
    }
}
    

/**
 * @brief sessionȫ�ֹ�����
 * @return ȫ�־��ָ��
 */
NtfyObjMgr* NtfyObjMgr::_instance = NULL;
NtfyObjMgr* NtfyObjMgr::Instance (void)
{
    if (NULL == _instance)
    {
        _instance = new NtfyObjMgr;
    }

    return _instance;
}

/**
 * @brief session����ȫ�ֵ����ٽӿ�
 */
void NtfyObjMgr::Destroy()
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
NtfyObjMgr::NtfyObjMgr()
{
}

/**
 * @brief ��������, ��������Դ, ������������
 */
NtfyObjMgr::~NtfyObjMgr()
{
}

/**
 * @brief ע�᳤����session��Ϣ
 * @param session_name �����ӵı�ʶ, ÿ�����Ӵ���һ��session��װ��ʽ
 * @param session �����Ӷ���ָ��, ������������
 * @return 0 �ɹ�, < 0 ʧ��
 */
int NtfyObjMgr::RegisterSession(int session_name, ISessionNtfy* session)
{
    if (session_name <= 0 || NULL == session) {
        MTLOG_ERROR("session %d, register %p failed", session_name, session);
        return -1;
    }

    SessionMap::iterator it = _session_map.find(session_name);
    if (it != _session_map.end())
    {
        MTLOG_ERROR("session %d, register %p already", session_name, session);
        return -2;
    }

    _session_map.insert(SessionMap::value_type(session_name, session));

    return 0;
}

/**
 * @brief ��ȡע�᳤����session��Ϣ
 * @param session_name �����ӵı�ʶ, ÿ�����Ӵ���һ��session��װ��ʽ
 * @return ������ָ��, ʧ��ΪNULL
 */
ISessionNtfy* NtfyObjMgr::GetNameSession(int session_name)
{
    SessionMap::iterator it = _session_map.find(session_name);
    if (it != _session_map.end())
    {
        return it->second;
    } 
    else
    {
        return NULL;
    }
}

/**
 * @brief ��ȡͨ��֪ͨ����, ���߳�֪ͨ������session֪ͨ�������
 * @param type ����, �߳�֪ͨ���ͣ�UDP/TCP SESSION֪ͨ��
 * @param session_name proxyģ��,һ����ȡsession����
 * @return ֪ͨ�����ָ��, ʧ��ΪNULL
 */
EpollerObj* NtfyObjMgr::GetNtfyObj(int type, int session_name)
{
    EpollerObj* obj = NULL;
    SessionProxy* proxy = NULL;    

    switch (type)
    {
        case NTFY_OBJ_THREAD:
            obj = _fd_ntfy_pool.AllocPtr();
            break;

        case NTFY_OBJ_SESSION:
            proxy = _udp_proxy_pool.AllocPtr();
            obj = proxy;
            break;

        case NTFY_OBJ_KEEPALIVE:    // no need get this now
            break;

        default:
            break;
    }

    // ��ȡ�ײ�ĳ����Ӷ���, ����������ʵ�ʵ�֪ͨ����
    if (proxy) {
        ISessionNtfy* ntfy = this->GetNameSession(session_name);
        if (!ntfy) {
            MTLOG_ERROR("ntfy get session name(%d) failed", session_name);
            this->FreeNtfyObj(proxy);
            obj = NULL;
        } else {
            proxy->SetRealNtfyObj(ntfy);
        }
    }    

    return obj;

}

/**
 * @brief �ͷ�֪ͨ����ָ��
 * @param obj ֪ͨ����
 */
void NtfyObjMgr::FreeNtfyObj(EpollerObj* obj)
{
    SessionProxy* proxy = NULL;    
    if (!obj) {
        return;
    }

    int type = obj->GetNtfyType();
    obj->Reset();
    
    switch (type)
    {
        case NTFY_OBJ_THREAD:
            return _fd_ntfy_pool.FreePtr(obj);
            break;

        case NTFY_OBJ_SESSION:
            proxy = dynamic_cast<SessionProxy*>(obj);
            return _udp_proxy_pool.FreePtr(proxy);
            break;

        case NTFY_OBJ_KEEPALIVE:
            break;

        default:
            break;
    }

    delete obj;
    return;
}



