/**
 *  @filename epoll_proxy.h
 *  @info     epoll for micro thread manage
 */

#ifndef _EPOLL_PROXY___
#define _EPOLL_PROXY___

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <assert.h>

#include <set>
#include <vector>
using std::set;
using std::vector;

#define  epoll_assert(statement)
//#define  epoll_assert(statement)   assert(statement)


namespace NS_MICRO_THREAD {


/******************************************************************************/
/*  ����ϵͳͷ�ļ����䶨��                                                    */
/******************************************************************************/

/**
 * @brief add more detail for linux <sys/queue.h>, freebsd and University of California 
 * @info  queue.h version 8.3 (suse)  diff version 8.5 (tlinux)
 */
#ifndef TAILQ_CONCAT

#define TAILQ_EMPTY(head)   ((head)->tqh_first == NULL)
#define TAILQ_FIRST(head)   ((head)->tqh_first)
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define TAILQ_LAST(head, headname) \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))

#define TAILQ_FOREACH(var, head, field)                                     \
        for ((var) = TAILQ_FIRST((head));                                   \
             (var);                                                         \
             (var) = TAILQ_NEXT((var), field))

#define TAILQ_CONCAT(head1, head2, field)                                   \
do {                                                                        \
    if (!TAILQ_EMPTY(head2)) {                                              \
        *(head1)->tqh_last = (head2)->tqh_first;                            \
        (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;             \
        (head1)->tqh_last = (head2)->tqh_last;                              \
        TAILQ_INIT((head2));                                                \
    }                                                                       \
} while (0)

#endif    

#ifndef TAILQ_FOREACH_SAFE      // tlinux no this define    
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                          \
        for ((var) = TAILQ_FIRST((head));                                   \
             (var) && ((tvar) = TAILQ_NEXT((var), field), 1);               \
             (var) = (tvar))  
#endif



/******************************************************************************/
/*  Epoll proxy ������ʵ�ֲ���                                                */
/******************************************************************************/

class EpollProxy;
class MicroThread;

/**
 *  @brief epoll֪ͨ������ඨ��
 */
class EpollerObj
{
protected:
    int _fd;                ///< ϵͳFD �� socket
    int _events;            ///< �������¼�����
    int _revents;           ///< �յ����¼�����
    int _type;              ///< ���������
    MicroThread* _thread;   ///< �����߳�ָ�����

public:

    TAILQ_ENTRY(EpollerObj) _entry;       ///< ����΢�̵߳Ĺ������
    
    /**
     *  @brief ��������������
     */
    explicit EpollerObj(int fd = -1) {
        _fd      = fd;
        _events  = 0;
        _revents = 0;
        _type    = 0;
        _thread  = NULL;
    };
    virtual ~EpollerObj(){};

    /**
     *  @brief �ɶ��¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
     *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
     */
    virtual int InputNotify();
    
    /**
     *  @brief ��д�¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
     *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
     */
    virtual int OutputNotify();
    
    /**
     *  @brief �쳣֪ͨ�ӿ�
     *  @return ���Է���ֵ, ���������¼�����
     */
    virtual int HangupNotify();

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlAdd(void* args);

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlDel(void* args);

    /**
     *  @brief fd�򿪿ɶ��¼�����
     */
    void EnableInput() {    _events |= EPOLLIN; };

    /**
     *  @brief fd�򿪿�д�¼�����
     */
    void EnableOutput() {     _events |= EPOLLOUT; };

    /**
     *  @brief fd�رտɶ��¼�����
     */
    void DisableInput() {   _events &= ~EPOLLIN; };

    /**
     *  @brief fd�رտ�д�¼�����
     */
    void DisableOutput() {    _events &= ~EPOLLOUT; };

    /**
     *  @brief ϵͳsocket���ö�ȡ��װ
     */
    int GetOsfd() { return _fd; };
    void SetOsfd(int fd) {   _fd = fd; };

    /**
     *  @brief �����¼����յ��¼��ķ��ʷ���
     */
    int GetEvents() { return _events; };
    void SetRcvEvents(int revents) { _revents = revents; };
    int GetRcvEvents() { return _revents; };

    /**
     *  @brief ����������, ��ȡ��ʵ����
     */
    int GetNtfyType() {    return _type; };
    virtual void Reset() {
        _fd      = -1;
        _events  = 0;
        _revents = 0;
        _type    = 0;
        _thread  = NULL;
    };
        
    /**
     *  @brief �������ȡ������΢�߳̾���ӿ�
     *  @param thread �������߳�ָ��
     */
    void SetOwnerThread(MicroThread* thread) {      _thread = thread; };
    MicroThread* GetOwnerThread() {        return _thread; };
    
};

typedef TAILQ_HEAD(__EpFdList, EpollerObj) EpObjList;  ///< ��Ч��˫������ 
typedef struct epoll_event EpEvent;                 ///< �ض���һ��epoll event


/**
 *  @brief EPOLL֧��ͬһFD����߳�����, ����һ�����ü�������, Ԫ�ض���
 *  @info  ���ü����״�����, û��ʵ������, �ֶα���, �����Ƴ��� 20150623
 */
class FdRef
{
private:
    int _wr_ref;             ///< ����д�����ü���
    int _rd_ref;             ///< �����������ü���
    int _events;             ///< ��ǰ�����������¼��б�
    int _revents;            ///< ��ǰ��fd�յ����¼���Ϣ, ����epoll_wait��������Ч
    EpollerObj* _epobj;      ///< ����ע�����������һ��fd����һ������

public:

    /**
     *  @brief ��������������
     */
    FdRef() {
        _wr_ref  = 0;
        _rd_ref  = 0;
        _events  = 0;
        _revents = 0;
        _epobj   = NULL;
    };
    ~FdRef(){};

    /**
     *  @brief �����¼���ȡ�����ýӿ�
     */
    void SetListenEvents(int events) {
        _events = events;
    };
    int GetListenEvents() {
        return _events;
    };

    /**
     *  @brief ���������ȡ�����ýӿ�
     */
    void SetNotifyObj(EpollerObj* ntfy) {
        _epobj = ntfy;
    };
    EpollerObj* GetNotifyObj() {
        return _epobj;
    };

    /**
     *  @brief �������ü����ĸ���
     */
    void AttachEvents(int event) {
        if (event & EPOLLIN) {
            _rd_ref++;
        }
        if (event & EPOLLOUT){
            _wr_ref++;
        }
    };
    void DetachEvents(int event) {
        if (event & EPOLLIN) {
            if (_rd_ref > 0) {
                _rd_ref--;
            } else {
                _rd_ref = 0;
            }
        }
        if (event & EPOLLOUT){
            if (_wr_ref > 0) {
                _wr_ref--;
            } else {
                _wr_ref = 0;
            }
        }
    };

    /**
     * @brief ��ȡ���ü���
     */
    int ReadRefCnt() { return _rd_ref; };
    int WriteRefCnt() { return _wr_ref; };
    
};


/**
 *  @brief EPOLL����, ��װepoll������epollȫ������
 */
class EpollProxy
{
public:
    static const int DEFAULT_MAX_FD_NUM = 100000;   ///< Ĭ������ص�fd
    
private:  
    int                 _epfd;                      ///< epoll �����
    int                 _maxfd;                     ///< �����ļ������    
    EpEvent*            _evtlist;                   ///< epoll���ظ��û����¼��б�ָ��
    FdRef*              _eprefs;                    ///< �û��������¼����ع�������
    
public:  

    /**
     *  @brief ��������������
     */
    EpollProxy();
    virtual ~EpollProxy(){};

    /**
     *  @brief epoll��ʼ������ֹ����, ���붯̬�ڴ��
     *  @param max_num ���ɹ����fd��Ŀ
     */
    int InitEpoll(int max_num);
    void TermEpoll(void);

    /**
     *  @brief epoll_wait ��ȡ���ȴ�ʱ��ӿ�
     *  @return Ŀǰ��Ҫ�ȴ���ʱ��, ��λMS
     */
    virtual int EpollGetTimeout(void) {     return 0;};

    /**
     *  @brief epoll �������Ƚӿ�
     *  @param fdlist ��·��������, ���з��͵�socket����
     *  @param fd    ����socket, �����ȴ�
     *  @param timeout ��ʱʱ������, ����
     *  @return true �ɹ�, false ʧ��
     */
    virtual bool EpollSchedule(EpObjList* fdlist, EpollerObj* fd, int timeout) { return false;};
    
    /**
     *  @brief ��һ��΢�߳�����������socket����epoll����
     *  @param fdset ΢�߳�������socket����
     *  @return true �ɹ�, false ʧ��
     */
    bool EpollAdd(EpObjList& fdset);

    /**
     *  @brief ��һ��΢�߳�����������socket�Ƴ�epoll����
     *  @param fdset ΢�߳�������socket����
     *  @return true �ɹ�, false ʧ��
     */
    bool EpollDel(EpObjList& fdset);

    /**
     *  @brief epoll_wait �Լ��ַ��������
     */
    void EpollDispath(void);

    /**
     *  @brief ����һ��fdע��, ���������¼�
     *  @param fd �ļ�������¼���Ϣ
     *  @param obj epoll�ص�����
     */
    bool EpollAddObj(EpollerObj* obj);

    /**
     *  @brief ȡ��һ��fdע��, ���������¼�
     *  @param fd �ļ�������¼���Ϣ
     *  @param obj epoll�ص�����
     */
    bool EpollDelObj(EpollerObj* obj);

    /**
     * @brief ��װepoll ctl�Ĵ����뵱ǰ�����¼��ļ�¼, �ڲ��ӿ�
     * @param fd �������ļ����
     * @param new_events ��Ҫ�����ļ����¼�
     */
    bool EpollCtrlAdd(int fd, int new_events);

    /**
     * @brief ��װepoll ctl�Ĵ����뵱ǰ�����¼��ļ�¼, �ڲ��ӿ�
     * @param fd �������ļ����
     * @param new_events ��Ҫ��ɾ���ļ����¼�
     */
    bool EpollCtrlDel(int fd, int new_events);    
    bool EpollCtrlDelRef(int fd, int new_events, bool use_ref);
    
    /**
     *  @brief ����fd��ȡ�������õĽṹ, ��fd���ɲ���, Ŀǰ�򵥹���
     *  @param fd �ļ�������
     *  @return �����ļ����ýṹ, NULL ��ʾʧ��
     */
    FdRef* FdRefGet(int fd) {
        return ((fd >= _maxfd) || (fd < 0)) ? (FdRef*)NULL : &_eprefs[fd];        
    };

    
    /**
     * @brief ������ע��ӿ�, ����ע���ȡ��ע��֪ͨ����
     * @param fd �������ļ����
     * @param obj ��ע���ȡ��ע��Ķ���
     */
    void EpollNtfyReg(int fd, EpollerObj* obj) {
        FdRef* ref = FdRefGet(fd);
        if (ref) {
            ref->SetNotifyObj(obj);
        }
    };

protected: 

    /**
     *  @brief ����ÿ��socket�����½����¼���Ϣ
     *  @param evtfdnum �յ��¼���fd������Ŀ
     */
    void EpollRcvEventList(int evtfdnum);

};
}//NAMESPCE

#endif


