/**
 *  @filename epoll_proxy.cpp
 *  @info     epoll for micro thread manage
 */

#include "epoll_proxy.h"
#include "micro_thread.h"

using namespace NS_MICRO_THREAD;

/**
 *  @brief ���캯��
 */
EpollProxy::EpollProxy()
{
    _maxfd = EpollProxy::DEFAULT_MAX_FD_NUM;
    _epfd = -1;
    _evtlist = NULL;
    _eprefs = NULL;
}

/**
 *  @brief epoll��ʼ��, ���붯̬�ڴ��
 */
int EpollProxy::InitEpoll(int max_num)
{
    int rc = 0;
    if (max_num > _maxfd)   // ������õ���Ŀ�ϴ�, ��������fd��Ŀ
    {
        _maxfd = max_num;
    }
    
    _epfd =  epoll_create(_maxfd);
    if (_epfd < 0)
    {
        rc = -1;
        goto EXIT_LABEL;
    }
    fcntl(_epfd, F_SETFD, FD_CLOEXEC);

    _eprefs = new FdRef[_maxfd];
    if (NULL == _eprefs)
    {
        rc = -2;
        goto EXIT_LABEL;
    }

    _evtlist = (EpEvent*)calloc(_maxfd, sizeof(EpEvent));
    if (NULL == _evtlist)
    {
        rc = -3;
        goto EXIT_LABEL;
    }
    
    struct rlimit rlim;
    memset(&rlim, 0, sizeof(rlim));
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
    {
        if ((int)rlim.rlim_max < _maxfd)
        {
            rlim.rlim_cur = rlim.rlim_max;
            setrlimit(RLIMIT_NOFILE, &rlim);
            rlim.rlim_cur = _maxfd;
            rlim.rlim_max = _maxfd;
            setrlimit(RLIMIT_NOFILE, &rlim);
        } 
    }

EXIT_LABEL:

    if (rc < 0)
    {
        TermEpoll();
    }

    return rc;
}

/**
 *  @brief epoll����ʼ��
 */
void EpollProxy::TermEpoll()
{
    if (_epfd > 0)
    {
        close(_epfd);
        _epfd = -1;
    }
    
    if (_evtlist != NULL)
    {
        free(_evtlist);
        _evtlist = NULL;
    }
    
    if (_eprefs != NULL)
    {
        delete []_eprefs;
        _eprefs = NULL;
    }
}

/**
 *  @brief ��һ��΢�߳�����������socket����epoll����
 *  @param fdset ΢�߳�������socket����
 *  @return true �ɹ�, false ʧ��, ʧ�ܻᾡ���ع�, ����Ӱ��
 */
bool EpollProxy::EpollAdd(EpObjList& obj_list)
{
    bool ret = true;
    EpollerObj *epobj = NULL;
    EpollerObj *epobj_error = NULL;
    TAILQ_FOREACH(epobj, &obj_list, _entry)
    {
        if (!EpollAddObj(epobj))
        {
            MTLOG_ERROR("epobj add failed, fd: %d", epobj->GetOsfd());
            epoll_assert(0);
            epobj_error = epobj;
            ret = false;
            goto EXIT_LABEL;
        }
    }

EXIT_LABEL:

    if (!ret)
    {
        TAILQ_FOREACH(epobj, &obj_list, _entry)
        {
            if (epobj == epobj_error)
            {
                break;
            }
            EpollDelObj(epobj);
        }
    }

    return ret;
}


/**
 *  @brief ��һ��΢�߳�����������socket�Ƴ�epoll����
 *  @param fdset ΢�߳�������socket����
 *  @return true �ɹ�, false ʧ��
 */
bool EpollProxy::EpollDel(EpObjList& obj_list)
{
    bool ret = true;
    
    EpollerObj *epobj = NULL;
    TAILQ_FOREACH(epobj, &obj_list, _entry)
    {
        if (!EpollDelObj(epobj))  // failed also need continue, be sure ref count ok
        {
            MTLOG_ERROR("epobj del failed, fd: %d", epobj->GetOsfd());
            epoll_assert(0);
            ret = false;
        }
    }

    return ret;
}


/**
 * @brief ����epfd����epctrl, �ɹ���Ҫ���µ�ǰ�����¼�ֵ
 */
bool EpollProxy::EpollCtrlAdd(int fd, int events)
{
    FdRef* item = FdRefGet(fd);
    if (NULL == item)
    {
        MT_ATTR_API(320851, 1); // fd error
        MTLOG_ERROR("epfd ref not find, failed, fd: %d", fd);
        epoll_assert(0);
        return false;
    }

    // �������ü���, �������̻������ü���, ʧ��Ҫ�ع�
    item->AttachEvents(events);
    
    int old_events = item->GetListenEvents();
    int new_events = old_events | events;
    if (old_events == new_events) {
        return true;
    }

    int op = old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    EpEvent ev;
    ev.events = new_events;
    ev.data.fd = fd;
    if ((epoll_ctl(_epfd, op, fd, &ev) < 0) && !(op == EPOLL_CTL_ADD && errno == EEXIST))
    {
        MT_ATTR_API(320850, 1); // epoll error
        MTLOG_ERROR("epoll ctrl failed, fd: %d, op: %d, errno: %d", fd, op, errno);
        item->DetachEvents(events);
        epoll_assert(0);
        return false;
    } 
    item->SetListenEvents(new_events);
    
    return true;

}

/**
 * @brief ����epfd����epctrl, �ɹ���Ҫ���µ�ǰ�����¼�ֵ
 */
bool EpollProxy::EpollCtrlDel(int fd, int events)
{
    return EpollCtrlDelRef(fd, events, false);
}

/**
 * @brief ����epfd����epctrl, ������ü���, ����Ԥ�賤����, ����ÿ�ζ�epollctl
 */
bool EpollProxy::EpollCtrlDelRef(int fd, int events, bool use_ref)
{
    FdRef* item = FdRefGet(fd);
    if (NULL == item)
    {
        MT_ATTR_API(320851, 1); // fd error
        MTLOG_ERROR("epfd ref not find, failed, fd: %d", fd);
        epoll_assert(0);
        return false;
    }

    item->DetachEvents(events);  // delete ʧ�ܲ��ع�����
    int old_events = item->GetListenEvents();
    int new_events = old_events &~ events;  // Ĭ�����

    // ���Ҫ������ɾ��, ��Ҫ�˲��Ƿ�����ɾ������
    if (use_ref)
    {
        new_events = old_events;
        if (0 == item->ReadRefCnt()) {
            new_events = new_events & ~EPOLLIN;
        }
        if (0 == item->WriteRefCnt()) {
            new_events = new_events & ~EPOLLOUT;
        }
    }

    if (old_events == new_events)
    {
        return true;
    }
    
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    EpEvent ev;
    ev.events = new_events;
    ev.data.fd = fd;
    if ((epoll_ctl(_epfd, op, fd, &ev) < 0) && !(op == EPOLL_CTL_DEL && errno == ENOENT))
    {
        MT_ATTR_API(320850, 1); // epoll error
        MTLOG_ERROR("epoll ctrl failed, fd: %d, op: %d, errno: %d", fd, op, errno);
        epoll_assert(0);
        return false;
    }    
    item->SetListenEvents(new_events);
    
    return true;
}



/**
 * @brief ����epfd����epctrl, ���ʧ��, ��������
 */
bool EpollProxy::EpollAddObj(EpollerObj* obj)
{
    if (NULL == obj)
    {
        MTLOG_ERROR("epobj input invalid, %p", obj);
        return false;
    }

    FdRef* item = FdRefGet(obj->GetOsfd());
    if (NULL == item)
    {    
        MT_ATTR_API(320851, 1); // fd error
        MTLOG_ERROR("epfd ref not find, failed, fd: %d", obj->GetOsfd());
        epoll_assert(0);
        return false;
    }

    // ��ͬ�Ļص�״̬, ��ͬ�ķ�ʽ���� del �¼�, �������Ӹ��÷�ʽ�Ĵ�������
    int ret = obj->EpollCtlAdd(item);
    if (ret < 0)
    {
        MTLOG_ERROR("epoll ctrl callback failed, fd: %d, obj: %p", obj->GetOsfd(), obj);
        epoll_assert(0);
        return false;
    }

    return true;
}


/**
 *  @brief ��һ��΢�߳�����������socket�Ƴ�epoll����
 *  @param fdset ΢�߳�������socket����
 *  @return true �ɹ�, false ʧ��
 */
bool EpollProxy::EpollDelObj(EpollerObj* obj)
{
    if (NULL == obj)
    {
        MTLOG_ERROR("fdobj input invalid, %p", obj);
        return false;
    }
    
    FdRef* item = FdRefGet(obj->GetOsfd());
    if (NULL == item)
    {    
        MT_ATTR_API(320851, 1); // fd error
        MTLOG_ERROR("epfd ref not find, failed, fd: %d", obj->GetOsfd());
        epoll_assert(0);
        return false;
    }
    
    // ��ͬ�Ļص�״̬, ��ͬ�ķ�ʽ���� del �¼�, �������Ӹ��÷�ʽ�Ĵ�������
    int ret = obj->EpollCtlDel(item);
    if (ret < 0)
    {
        MTLOG_ERROR("epoll ctrl callback failed, fd: %d, obj: %p", obj->GetOsfd(), obj);
        epoll_assert(0);
        return false;
    }

    return true;
}


/**
 *  @brief ����ÿ��socket�����½����¼���Ϣ
 *  @param evtfdnum �յ��¼���fd������Ŀ
 */
void EpollProxy::EpollRcvEventList(int evtfdnum)
{
    int ret = 0;
    int osfd = 0;
    int revents = 0;
    FdRef* item = NULL;
    EpollerObj* obj = NULL;

    for (int i = 0; i < evtfdnum; i++)
    {
        osfd = _evtlist[i].data.fd;
        item = FdRefGet(osfd);
        if (NULL == item)
        {
            MT_ATTR_API(320851, 1); // fd error
            MTLOG_ERROR("epfd ref not find, failed, fd: %d", osfd);
            epoll_assert(0);
            continue;
        }
        
        revents = _evtlist[i].events;
        obj = item->GetNotifyObj();
        if (NULL == obj) 
        {
            MTLOG_ERROR("fd notify obj null, failed, fd: %d", osfd);
            EpollCtrlDel(osfd, (revents & (EPOLLIN | EPOLLOUT)));
            continue;
        }
        obj->SetRcvEvents(revents);

        // 1. ������, ��Ϻ�ֱ������
        if (revents & (EPOLLERR | EPOLLHUP))
        {
            obj->HangupNotify();
            continue;
        }

        // 2. �ɶ��¼�, ��0����ֵ������
        if (revents & EPOLLIN) {
            ret = obj->InputNotify();
            if (ret != 0) {
                continue;
            }
        }

        // 3. ��д�¼�, ��0����ֵ������
        if (revents & EPOLLOUT) {
            ret = obj->OutputNotify();
            if (ret != 0) {
                continue;
            }
        }
    }
}


/**
 *  @brief epoll_wait �Լ��ַ��������
 */
void EpollProxy::EpollDispath()
{
    int wait_time = EpollGetTimeout();
    int nfd = epoll_wait(_epfd, _evtlist, _maxfd, wait_time);
    if (nfd <= 0) {
        return;
    }

    EpollRcvEventList(nfd);    
}


/**
 *  @brief �ɶ��¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int EpollerObj::InputNotify()
{
    MicroThread* thread = this->GetOwnerThread();
    if (NULL == thread) 
    {
        epoll_assert(0);
        MTLOG_ERROR("Epoll fd obj, no thread ptr, wrong");
        return -1;
    }

    // ����¼�ͬʱ����, ���ظ�����
    if (thread->HasFlag(MicroThread::IO_LIST))
    {
        MtFrame* frame = MtFrame::Instance();
        frame->RemoveIoWait(thread);
        frame->InsertRunable(thread);
    }

    return 0;    
}

/**
 *  @brief ��д�¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
 *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
 */
int EpollerObj::OutputNotify()
{
    MicroThread* thread = this->GetOwnerThread();
    if (NULL == thread) 
    {
        epoll_assert(0);
        MTLOG_ERROR("Epoll fd obj, no thread ptr, wrong");
        return -1;
    }

    // ����¼�ͬʱ����, ���ظ�����
    if (thread->HasFlag(MicroThread::IO_LIST))
    {
        MtFrame* frame = MtFrame::Instance();
        frame->RemoveIoWait(thread);
        frame->InsertRunable(thread);
    }

    return 0;    
}

/**
 *  @brief �쳣֪ͨ�ӿ�, �ر�fd����, thread�ȴ�����ʱ
 *  @return ���Է���ֵ, ���������¼�����
 */
int EpollerObj::HangupNotify()
{
    MtFrame* frame = MtFrame::Instance();
    frame->EpollCtrlDel(this->GetOsfd(), this->GetEvents());
    return 0;
}

/**
 *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
 *  @param args fd���ö����ָ��
 *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
 */
int EpollerObj::EpollCtlAdd(void* args)
{
    MtFrame* frame = MtFrame::Instance();
    FdRef* fd_ref = (FdRef*)args;
    epoll_assert(fd_ref != NULL);

    int osfd = this->GetOsfd();
    int new_events = this->GetEvents();

    // ֪ͨ������Ҫ����, FD֪ͨ���������ϲ��Ḵ��, ��������ͻ���, �쳣log��¼
    EpollerObj* old_obj = fd_ref->GetNotifyObj();
    if ((old_obj != NULL) && (old_obj != this))
    {
        MTLOG_ERROR("epfd ref conflict, fd: %d, old: %p, now: %p", osfd, old_obj, this);
        return -1;
    }
    fd_ref->SetNotifyObj(this);

    // ���ÿ�ܵ�epoll ctl�ӿ�, ����epoll ctrlϸ��
    if (!frame->EpollCtrlAdd(osfd, new_events))
    {
        MTLOG_ERROR("epfd ref add failed, log");
        fd_ref->SetNotifyObj(old_obj);
        return -2;
    }

    return 0;
}

/**
 *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
 *  @param args fd���ö����ָ��
 *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
 */
int EpollerObj::EpollCtlDel(void* args)
{
    MtFrame* frame = MtFrame::Instance();
    FdRef* fd_ref = (FdRef*)args;
    epoll_assert(fd_ref != NULL);

    int osfd = this->GetOsfd();
    int events = this->GetEvents();
    
    // ֪ͨ������Ҫ����, FD֪ͨ���������ϲ��Ḵ��, ��������ͻ���, �쳣log��¼
    EpollerObj* old_obj = fd_ref->GetNotifyObj();
    if (old_obj != this)
    {
        MTLOG_ERROR("epfd ref conflict, fd: %d, old: %p, now: %p", osfd, old_obj, this);
        return -1;
    }
    fd_ref->SetNotifyObj(NULL);

    // ���ÿ�ܵ�epoll ctl�ӿ�, ����epoll ctrlϸ��
    if (!frame->EpollCtrlDelRef(osfd, events, false)) // �����з���, �״�����, �رյ�
    {
        MTLOG_ERROR("epfd ref del failed, log");
        fd_ref->SetNotifyObj(old_obj);
        return -2;
    }

    return 0;
    
}


