/**
 *  @filename micro_thread.h
 *  @info  micro thread manager
 */

#ifndef ___MICRO_THREAD_H__
#define ___MICRO_THREAD_H__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>

#include <set>
#include <vector>
#include <queue>
#include "heap.h"
#include "epoll_proxy.h"
#include "heap_timer.h"

using std::vector;
using std::set;
using std::queue;

namespace NS_MICRO_THREAD {

#define STACK_PAD_SIZE      128         ///< ջ���¸�������Ĵ�С
#define MEM_PAGE_SIZE       4096        ///< �ڴ�ҳĬ�ϴ�С
#define DEFAULT_STACK_SIZE  128*1024    ///< Ĭ��ջ��С128K
#define DEFAULT_THREAD_NUM  2000        ///< Ĭ��2000����ʼ�߳�

typedef unsigned long long  utime64_t;  ///< 64λ��ʱ�䶨��
typedef void (*ThreadStart)(void*);      ///< ΢�߳���ں�������

/**
 * @brief �̵߳��ȵ����������, �������С�ӿڷ�װ
 */
class ScheduleObj
{
public:

    /**
     * @brief ��������ʾ�����
     */
    static ScheduleObj* Instance (void); 

    /**
     * @brief ��ȡȫ�ֵ�ʱ���, ���뵥λ
     */
    utime64_t ScheduleGetTime(void);    

    /**
     * @brief ��������΢�߳�������
     */
    void ScheduleThread(void);

    /**
     * @brief �̵߳�����������sleep״̬
     */
    void ScheduleSleep(void);

    /**
     * @brief �̵߳�����������pend״̬
     */
    void SchedulePend(void);

    /**
     * @brief �̵߳���ȡ��pend״̬, �ⲿ����ȡ��
     */
    void ScheduleUnpend(void* thread);

    /**
     * @brief �߳�ִ����Ϻ�, ���մ���
     */
    void ScheduleReclaim(void);

    /**
     * @brief ���������ȳ�ʼִ��
     */
    void ScheduleStartRun(void);

private:
    static ScheduleObj* _instance;   // ˽�о�� 
};


/**
 * @brief �߳�ͨ�õ�ջ֡�ṹ����
 */
struct MtStack
{
    int  _stk_size;              ///< ջ�Ĵ�С, ��Чʹ�ÿռ�
    int  _vaddr_size;            ///< �����buff�ܴ�С
    char *_vaddr;                ///< ������ڴ����ַ
    void *_esp;                  ///< ջ��esp�Ĵ���
    char *_stk_bottom;           ///< ջ��͵ĵ�ַ�ռ�
    char *_stk_top;              ///< ջ��ߵĵ�ַ�ռ�
    void *_private;              ///< �߳�˽������
	int valgrind_id;			 ///< valgrind id
};


/**
 * @brief ͨ�õ��߳�ģ�Ͷ���
 */
class Thread : public  HeapEntry
{
public:

    /**
     * @brief ��������������
     */
    explicit Thread(int stack_size = 0);
    virtual ~Thread(){};

    /**
     * @brief �̵߳�ʵ�ʹ�������
     */
    virtual void Run(void){};

    /**
     * @brief ��ʼ���߳�,���ջ�������ĳ�ʼ��
     */
    bool Initial(void);

    /**
     * @brief ��ֹ�߳�,���ջ���������ͷ�
     */
    void Destroy(void);

    /**
     * @brief �߳�״̬����, �ɸ���״̬
     */
    void Reset(void);

    /**
     * @brief �߳���������˯��, ��λ����
     * @param ms ˯�ߺ�����
     */
    void sleep(int ms); 

    /**
     * @brief �߳���������ȴ�, �ö����߳�������
     */
    void Wait();

    /**
     * @brief �����л�, ����״̬, ��������
     */
    void SwitchContext(void);

    /**
     * @brief �ָ�������, �л�����
     */
    void RestoreContext(void);

    /**
     * @brief ��ȡ�����ʱ��
     * @return �̵߳Ļ���ʱ���
     */
    utime64_t GetWakeupTime(void) { 
        return _wakeup_time; 
    };

    /**
     * @brief ���������ʱ��
     * @param waketime �̵߳Ļ���ʱ���
     */
    void SetWakeupTime(utime64_t waketime) { 
        _wakeup_time = waketime;
    };

    /**
     * @brief �����߳�˽������
     * @param data �߳�˽������ָ�룬ʹ�������Լ������ڴ棬����ֻ����ָ��
     */
    void SetPrivate(void *data)
    {
        _stack->_private = data;
    }
    
    /**
     * @brief ��ȡ�߳�˽������
     */
    void* GetPrivate()
    {
        return _stack->_private;
    }

	/**
     * @brief ��ʼ��������,���üĴ���,��ջ
     */
    bool CheckStackHealth(char *esp);

protected: 

    /**
     * @brief �����̴߳���״̬, ׼������
     */
    virtual void CleanState(void){};

    /**
     * @brief ��ʼ����ջ��Ϣ
     */
    virtual bool InitStack(void);

    /**
     * @brief �ͷŶ�ջ��Ϣ
     */
    virtual void FreeStack(void);

    /**
     * @brief ��ʼ��������,���üĴ���,��ջ
     */
    virtual void InitContext(void);
    
private:
    MtStack* _stack;        ///< ˽��ջָ��
    jmp_buf _jmpbuf;        ///< ������jmpbuff
    int _stack_size;        ///< ջ��С�ֶ�
    utime64_t _wakeup_time; ///< ˯�߻���ʱ��
};


/**
 * @brief ΢�߳����ݽṹ����
 */
class MicroThread : public Thread
{
public:
    enum ThreadType
    {
        NORMAL          =   0,   ///< Ĭ����ͨ�߳�, û�ж�̬�����ջ��Ϣ
        PRIMORDIAL      =   1,   ///< ԭ���߳�, main��������
        DAEMON          =   2,   ///< �ػ��߳�, �ײ�IO EPOLL��������ȴ���
        SUB_THREAD      =   3,   ///< �����߳�, ��ִ�м򵥹���
    };
    
    enum ThreadFlag
    {
        NOT_INLIST	=  0x0,     ///< �޶���״̬
        FREE_LIST	=  0x1,     ///< ���ж�����
        IO_LIST		=  0x2,     ///< IO�ȴ�������
        SLEEP_LIST	=  0x4,     ///< ����SLEEP��
        RUN_LIST	=  0x8,     ///< �����ж�����
        PEND_LIST   =  0x10,    ///< ����������
        SUB_LIST    =  0x20,    ///< �����̶߳�����
        
    };

    enum ThreadState
    {
        INITIAL         =  0,   ///< ��ʼ��״̬
        RUNABLE         =  1,   ///< ������״̬
        RUNNING         =  2,   ///< ����������
        SLEEPING        =  3,   ///< IO�ȴ���SLEEP��
        PENDING         =  4,   ///< ����״̬��, �ȴ����߳�OK��
    };

    typedef TAILQ_ENTRY(MicroThread) ThreadLink;        ///< ΢�߳�����
    typedef TAILQ_HEAD(__ThreadSubTailq, MicroThread) SubThreadList;  ///< ΢�̶߳��ж���
    
public:   

    /**
     * @brief ΢�̹߳���������
     */
    MicroThread(ThreadType type = NORMAL);
    ~MicroThread(){};    
    
    ThreadLink _entry;          ///<  ״̬�������
    ThreadLink _sub_entry;      ///<  ���̶߳������

    /**
     * @brief ΢�̶߳�������ʵ��,������ʱ����絽������
     * @return �̵߳�ʵ�ʻ���ʱ��
     */
    virtual utime64_t HeapValue() {
        return GetWakeupTime();
    };

    /**
     * @brief �̵߳�ʵ�ʹ�������
     */
    virtual void Run(void);    
    
    /**
     * @breif fd����������в���
     */
    void ClearAllFd(void) {
        TAILQ_INIT(&_fdset);
    };
    void AddFd(EpollerObj* efpd) {
        TAILQ_INSERT_TAIL(&_fdset, efpd, _entry);
    };
    void AddFdList(EpObjList* fdset) {
        TAILQ_CONCAT(&_fdset, fdset, _entry);
    };    
    EpObjList& GetFdSet(void) {
        return _fdset;
    };

    /**
     * @breif ΢�߳����͹������
     */
    void SetType(ThreadType type) {
        _type = type;   
    }; 
    ThreadType GetType(void) {
        return _type;
    };

    /**
     * @breif ΢�߳����ͼ��ӿ�
     */
    bool IsDaemon(void) {
        return (DAEMON == _type);
    };
    bool IsPrimo(void) {
        return (PRIMORDIAL == _type);
    };
    bool IsSubThread(void) {
        return (SUB_THREAD == _type);
    };  

    /**
     * @brief  ���߳����������
     */
    void SetParent(MicroThread* parent) {
        _parent = parent;
    };
    MicroThread* GetParent() {
        return _parent;
    };
    void WakeupParent();

    /**
     * @brief  ���̵߳Ĺ���
     */
    void AddSubThread(MicroThread* sub);
    void RemoveSubThread(MicroThread* sub);
    bool HasNoSubThread();

    /**
     * @brief ΢�߳�����״̬����
     */
    void SetState(ThreadState state) {
        _state = state;   
    };
    ThreadState GetState(void) {
        return _state;
    }

    /**
     * @breif ΢�̱߳��λ����
     */
    void SetFlag(ThreadFlag flag) {
	_flag = (ThreadFlag)(_flag | flag);
    }; 
    void UnsetFlag(ThreadFlag flag) {
        _flag = (ThreadFlag)(_flag & ~flag);
    };    
    bool HasFlag(ThreadFlag flag) {
	return _flag & flag;
    };
    ThreadFlag GetFlag() {
        return _flag;
    };

    /**
     * @breif ΢�߳���ں�������ע��
     */    
    void SetSartFunc(ThreadStart func, void* args) {
        _start = func;
        _args  = args;
    };

    void* GetThreadArgs() {
        return _args;
    }
    
protected: 

    /**
     * @breif ΢�̸߳���״̬����
     */    
    virtual void CleanState(void);
    
private:    
    ThreadState _state;         ///< ΢�̵߳�ǰ״̬
    ThreadType _type;           ///< ΢�߳�����
    ThreadFlag _flag;           ///< ΢�̱߳��λ
    EpObjList _fdset;           ///< ΢�̹߳�ע��socket�б�
    SubThreadList _sub_list;    ///< �����̵߳Ķ���
    MicroThread* _parent;       ///< �����̵߳ĸ��߳�
    ThreadStart _start;         ///< ΢�߳�ע�ắ��
    void* _args;                ///< ΢�߳�ע�����

};
typedef std::set<MicroThread*> ThreadSet;       ///< ΢�߳�set����ṹ
typedef std::queue<MicroThread*> ThreadList;    ///< ΢�߳�queue����ṹ


/**
 * @brief ΢�߳���־�ӿ�, �ײ��, ��־�ɵ�����ע��
 */
class LogAdapter
{
public:

    /**
     * @brief ��־����������
     */
    LogAdapter(){};
    virtual ~LogAdapter(){};

    /**
     * @brief ��־���Ȱ��ȼ�����, ���ٽ��������Ŀ���
     * @return true ���Դ�ӡ�ü���, false ��������ӡ�ü���
     */
    virtual bool CheckDebug(){ return true;};
    virtual bool CheckTrace(){ return true;};
    virtual bool CheckError(){ return true;};

    /**
     * @brief ��־�ּ���¼�ӿ�
     */    
    virtual void LogDebug(char* fmt, ...){};
    virtual void LogTrace(char* fmt, ...){};
    virtual void LogError(char* fmt, ...){};

    /**
     * @brief �����ϱ��ӿ�
     */
    virtual void AttrReportAdd(int attr, int iValue){};
    virtual void AttrReportSet(int attr, int iValue){};
    
};


/**
 * @brief ΢�̳߳ؼ�ʵ��
 */
class ThreadPool
{
public:

    static unsigned int default_thread_num;   ///< Ĭ��2000΢�̴߳���
    static unsigned int default_stack_size;   ///< Ĭ��128Kջ��С 

    /**
     * @brief ����΢�̵߳���С������Ŀ
     */
    static void SetDefaultThreadNum(unsigned int num) {
        default_thread_num = num;   
    }; 

    /**
     * @brief ����΢�̵߳�Ĭ��ջ��С, ���ʼ��ǰ����
     */
    static void SetDefaultStackSize(unsigned int size) {
        default_stack_size = (size + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE * MEM_PAGE_SIZE;   
    }; 
    
    /**
     * @brief ΢�̳߳س�ʼ��
     */
    bool InitialPool(int max_num);

    /**
     * @brief ΢�̳߳ط���ʼ��
     */
    void DestroyPool (void); 

    /**
     * @brief ΢�̷߳���ӿ�
     * @return ΢�̶߳���
     */
    MicroThread* AllocThread(void);

    /**
     * @brief ΢�߳��ͷŽӿ�
     * @param thread ΢�̶߳���
     */
    void FreeThread(MicroThread* thread);

	/**
     * @brief ��ȡ��ǰ΢�߳�����
     * @param thread ΢�̶߳���
     */
    int GetUsedNum(void);
    
private:
    ThreadList      _freelist;      ///< ���д�����΢�̶߳���
    int             _total_num;     ///< Ŀǰ�ܵ�΢�߳���Ŀ�����������������
    int             _use_num;       ///< ��ǰ����ʹ�õ�΢�߳���Ŀ
    int             _max_num;       ///< ��󲢷�������, �����ڴ����ʹ��
};

typedef TAILQ_HEAD(__ThreadTailq, MicroThread) ThreadTailq;  ///< ΢�̶߳��ж���

/**
 * @brief ΢�߳̿����, ȫ�ֵĵ�����
 */
class MtFrame : public EpollProxy, public ThreadPool
{
private:
    static MtFrame* _instance;          ///< ����ָ��
    LogAdapter*     _log_adpt;          ///< ��־�ӿ�
	ThreadList      _runlist;           ///< ������queue, �����ȼ�
	ThreadTailq     _iolist;            ///< �ȴ����У������������� 
	ThreadTailq     _pend_list;         ///< �ȴ����У������������� 
	HeapList        _sleeplist;         ///< �ȴ���ʱ�Ķ�, ���������, ����ʱ��ȡ��С����
	MicroThread*    _daemon;            ///< �ػ��߳�, ִ��epoll wait, ��ʱ���
	MicroThread*    _primo;             ///< ԭ���߳�, ʹ�õ���ԭ����ջ
	MicroThread*    _curr_thread;       ///< ��ǰ�����߳�
	utime64_t       _last_clock;        ///< ȫ��ʱ���, ÿ��idle��ȡһ��
    int             _waitnum;           ///< �ȴ����е����߳���, �ɵ��ڵ��ȵĽ���
    CTimerMng*      _timer;             ///< TCP����ר�õ�timer��ʱ��
    int             _realtime;  /// < ʹ��ʵʱʱ��0, δ����

public:
    friend class ScheduleObj;           ///< ����������, �ǿ���������ģʽ, ��Ԫ����
    
public:  

    /**
     * @brief ΢�߳̿����, ȫ��ʵ����ȡ
     */
    static MtFrame* Instance (void);
    
    /**
     * @brief ΢�̰߳�����ϵͳIO���� sendto
     * @param fd ϵͳsocket��Ϣ
     * @param msg �����͵���Ϣָ��
     * @param len �����͵���Ϣ����
     * @param to Ŀ�ĵ�ַ��ָ��
     * @param tolen Ŀ�ĵ�ַ�Ľṹ����
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ͳ���, <0 ʧ��
     */
    static int sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, int tolen, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� recvfrom
     * @param fd ϵͳsocket��Ϣ
     * @param buf ������Ϣ������ָ��
     * @param len ������Ϣ����������
     * @param from ��Դ��ַ��ָ��
     * @param fromlen ��Դ��ַ�Ľṹ����
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ճ���, <0 ʧ��
     */
    static int recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� connect
     * @param fd ϵͳsocket��Ϣ
     * @param addr ָ��server��Ŀ�ĵ�ַ
     * @param addrlen ��ַ�ĳ���
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ͳ���, <0 ʧ��
     */
    static int connect(int fd, const struct sockaddr *addr, int addrlen, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� accept
     * @param fd �����׽���
     * @param addr �ͻ��˵�ַ
     * @param addrlen ��ַ�ĳ���
     * @param timeout ��ȴ�ʱ��, ����
     * @return >=0 accept��socket������, <0 ʧ��
     */
    static int accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� read
     * @param fd ϵͳsocket��Ϣ
     * @param buf ������Ϣ������ָ��
     * @param nbyte ������Ϣ����������
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ճ���, <0 ʧ��
     */
    static ssize_t read(int fd, void *buf, size_t nbyte, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� write
     * @param fd ϵͳsocket��Ϣ
     * @param buf ������Ϣ������ָ��
     * @param nbyte ������Ϣ����������
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ͳ���, <0 ʧ��
     */
    static ssize_t write(int fd, const void *buf, size_t nbyte, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� recv
     * @param fd ϵͳsocket��Ϣ
     * @param buf ������Ϣ������ָ��
     * @param len ������Ϣ����������
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ճ���, <0 ʧ��
     */
    static int recv(int fd, void *buf, int len, int flags, int timeout);

    /**
     * @brief ΢�̰߳�����ϵͳIO���� send
     * @param fd ϵͳsocket��Ϣ
     * @param buf �����͵���Ϣָ��
     * @param nbyte �����͵���Ϣ����
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ͳ���, <0 ʧ��
     */
    static ssize_t send(int fd, const void *buf, size_t nbyte, int flags, int timeout);


    /**
     * @brief ΢�߳�����sleep�ӿ�, ��λms
     */
    static void sleep(int ms);

    /**
     * @brief ΢�߳̽��ȴ��¼�,��������Ĳ���
     * @param fd ϵͳsocket��Ϣ
     * @param events �¼�����  EPOLLIN or EPOLLOUT
     * @param timeout ��ȴ�ʱ��, ����
     * @return >0 �ɹ����ճ���, <0 ʧ��
     */
    static int WaitEvents(int fd, int events, int timeout);

    /**
     * @brief ΢�̴߳����ӿ�
     * @param entry �߳���ں���
     * @param args  �߳���ڲ���
     * @return ΢�߳�ָ��, NULL��ʾʧ��
     */
    static MicroThread* CreateThread(ThreadStart entry, void *args, bool runable = true);

    /**
     * @brief �ػ��߳���ں���, ����ָ��Ҫ��static����
     * @param args  �߳���ڲ���
     */
    static void DaemonRun(void* args);

    /**
     * @brief ��ȡ��ǰ�̵߳ĸ��߳�
     */
    MicroThread *GetRootThread();

    /**
     * @brief ��ܳ�ʼ��, Ĭ�ϲ�����־����
     */
    bool InitFrame(LogAdapter* logadpt = NULL, int max_thread_num = 50000);

    /**
     * @brief HOOKϵͳapi������
     */
    void SetHookFlag();

    /**
     * @brief ��ܷ���ʼ��
     */
    void Destroy (void);
    
    /**
     * @brief ΢�߳̿�ܰ汾��ȡ
     */
    char* Version(void);

    /**
     * @brief ��ܻ�ȡȫ��ʱ���
     */
    utime64_t GetLastClock(void) {
    	if(_realtime)
    	{
        	return GetSystemMS();
        }	
        return _last_clock;
    };


    /**
     * @brief ��ܻ�ȡ��ǰ�߳�
     */
    MicroThread* GetActiveThread(void) {
        return _curr_thread;
    }; 

    /**
     * @brief ���ص�ǰ�����е��߳���, ֱ�Ӽ���, Ч�ʸ�
     * @return �ȴ��߳���
     */
    int RunWaitNum(void) {
        return _waitnum;        
    };

    /**
     * @brief ��ܱ�ע�����־�������
     */
    LogAdapter* GetLogAdpt(void) {
        return _log_adpt;
    };

    /**
     * @brief ��ȡ��ܱ��ʱ��ָ�� 
     */
    CTimerMng* GetTimerMng(void) {
        return _timer;
    };

    /**
     * @brief ��ܵ���epoll waitǰ, �ж��ȴ�ʱ����Ϣ
     */
    virtual int EpollGetTimeout(void);
    
    /**
     * @brief ΢�̴߳����л�����,���óɹ� ���ó�cpu, �ڲ��ӿ�
     * @param fdlist ��·������socket�б�
     * @param fd ���������fd��Ϣ
     * @param timeout ��ȴ�ʱ��, ����
     * @return true �ɹ�, false ʧ��
     */
    virtual bool EpollSchedule(EpObjList* fdlist, EpollerObj* fd, int timeout);    

    
    /**
     * @brief ΢�߳������л�, �ȴ������̵߳Ļ���
     * @param timeout ��ȴ�ʱ��, ����
     */
    void WaitNotify(utime64_t timeout);

    /**
     * @brief ��ܹ����̵߳�Ԫ, �Ƴ�IO�ȴ�״̬, �ڲ��ӿ�
     * @param thread ΢�̶߳���
     */
    void RemoveIoWait(MicroThread* thread);    

    /**
     * @brief ��ܹ����̵߳�Ԫ, ��������ж���, �ڲ��ӿ�
     * @param thread ΢�̶߳���
     */
    void InsertRunable(MicroThread* thread);

    /**
     * @brief ��ܹ����̵߳�Ԫ, ִ��pend�ȴ�״̬
     * @param thread ΢�̶߳���
     */
    void InsertPend(MicroThread* thread);
    
    /**
     * @brief ��ܹ����̵߳�Ԫ, �Ƴ�PEND�ȴ�״̬
     * @param thread ΢�̶߳���
     */
    void RemovePend(MicroThread* thread);

	void SetRealTime(int realtime_)
	{
		_realtime =realtime_;
	}
private:

    /**
     * @brief ΢�߳�˽�й���
     */
    MtFrame():_realtime(1){ _curr_thread = NULL; }; 

    /**
     * @brief ΢�߳�˽�л�ȡ�ػ��߳�
     */
    MicroThread* DaemonThread(void){
        return _daemon;
    };	

    /**
     * @brief ��ܵ����߳�����
     */
    void ThreadSchdule(void);

    /**
     * @brief ��ܴ���ʱ�ص�����
     */
    void CheckExpired();
    
    /**
     * @brief ��ܼ�⵽��ʱ, �������еĳ�ʱ�߳�
     */
    void WakeupTimeout(void);
    
    /**
     * @brief ��ܸ���ȫ��ʱ���
     */
    void SetLastClock(utime64_t clock) {
        _last_clock = clock;
    };

    /**
     * @brief ������õ�ǰ�߳�
     */
    void SetActiveThread(MicroThread* thread) {
        _curr_thread = thread;
    };    

    /**
     * @brief ��ܵ�ʱ��Դ�ӿ�, ���غ��뼶��ʱ��
     */
    utime64_t GetSystemMS(void) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL);
    };

    /**
     * @brief ��ܹ����̵߳�Ԫ, ִ��IO�ȴ�״̬
     * @param thread ΢�̶߳���
     */
    void InsertSleep(MicroThread* thread);

    /**
     * @brief ��ܹ����̵߳�Ԫ, �Ƴ�IO�ȴ�״̬
     * @param thread ΢�̶߳���
     */
    void RemoveSleep(MicroThread* thread);

    /**
     * @brief ��ܹ����̵߳�Ԫ, ִ��IO�ȴ�״̬
     * @param thread ΢�̶߳���
     */
    void InsertIoWait(MicroThread* thread);

    /**
     * @brief ��ܹ����̵߳�Ԫ, �Ƴ������ж���
     * @param thread ΢�̶߳���
     */
    void RemoveRunable(MicroThread* thread);    

};

/**
 * @brief ��־��Ķ��岿��
 */
#define MTLOG_DEBUG(fmt, args...)                                              \
do {                                                                           \
       register NS_MICRO_THREAD::MtFrame *fm = NS_MICRO_THREAD::MtFrame::Instance(); \
       if (fm && fm->GetLogAdpt() && fm->GetLogAdpt()->CheckDebug())           \
       {                                                                       \
          fm->GetLogAdpt()->LogDebug((char*)"[%-10s][%-4d][%-10s]"fmt,         \
                __FILE__, __LINE__, __FUNCTION__, ##args);                     \
       }                                                                       \
} while (0)

#define MTLOG_TRACE(fmt, args...)                                              \
do {                                                                           \
       register NS_MICRO_THREAD::MtFrame *fm = NS_MICRO_THREAD::MtFrame::Instance(); \
       if (fm && fm->GetLogAdpt() && fm->GetLogAdpt()->CheckTrace())           \
       {                                                                       \
          fm->GetLogAdpt()->LogTrace((char*)"[%-10s][%-4d][%-10s]"fmt,         \
                __FILE__, __LINE__, __FUNCTION__, ##args);                     \
       }                                                                       \
} while (0)

#define MTLOG_ERROR(fmt, args...)                                              \
do {                                                                           \
       register NS_MICRO_THREAD::MtFrame *fm = NS_MICRO_THREAD::MtFrame::Instance(); \
       if (fm && fm->GetLogAdpt() && fm->GetLogAdpt()->CheckError())           \
       {                                                                       \
          fm->GetLogAdpt()->LogError((char*)"[%-10s][%-4d][%-10s]"fmt,         \
                __FILE__, __LINE__, __FUNCTION__, ##args);                     \
       }                                                                       \
} while (0)

#define MT_ATTR_API(ATTR, VALUE)                                               \
do {                                                                           \
       register NS_MICRO_THREAD::MtFrame *fm = NS_MICRO_THREAD::MtFrame::Instance(); \
       if (fm && fm->GetLogAdpt())                                             \
       {                                                                       \
          fm->GetLogAdpt()->AttrReportAdd(ATTR, VALUE);                        \
       }                                                                       \
} while (0)

#define MT_ATTR_API_SET(ATTR, VALUE)                                               \
	   do { 																		  \
			  register NS_MICRO_THREAD::MtFrame *fm = NS_MICRO_THREAD::MtFrame::Instance(); \
			  if (fm && fm->GetLogAdpt())											  \
			  { 																	  \
				 fm->GetLogAdpt()->AttrReportSet(ATTR, VALUE);						  \
			  } 																	  \
	   } while (0)



}// NAMESPACE NS_MICRO_THREAD

#endif

