/**
 *  @file mt_notify.h
 *  @info ΢�߳�ע���֪ͨ�����������
 *  @time 20130926
 **/

#ifndef __MT_NOTIFY_H__
#define __MT_NOTIFY_H__

#include <netinet/in.h>
#include <queue>
#include <map>
#include "mt_mbuf_pool.h"

namespace NS_MICRO_THREAD {

using std::queue;
using std::map;

class SessionProxy;
class TcpKeepConn;

/**
 * @brief ֪ͨ��������
 */
enum NTFY_OBJ_TYPE
{
    NTFY_OBJ_UNDEF     = 0,     ///< δ��������Ӷ���
    NTFY_OBJ_THREAD    = 1,     ///< �����Ӷ���, һ��fd��Ӧһ��thread
    NTFY_OBJ_KEEPALIVE = 2,     ///< TCP�������ֵ�notify����, ������ thread
    NTFY_OBJ_SESSION   = 3,     ///< UDP��sessionģ��, ����ĳ����Ӷ���
};

/**
 * @brief Э�����Ͷ���
 */
enum MULTI_PROTO 
{
    MT_UNKNOWN = 0,
    MT_UDP     = 0x1,                ///< �������� UDP
    MT_TCP     = 0x2                 ///< �������� TCP
};

/**
 * @brief ������sessionģ��, �����շ����ȹ���ӿ�
 */
typedef TAILQ_ENTRY(SessionProxy) NtfyEntry;
typedef TAILQ_HEAD(__NtfyList, SessionProxy) NtfyList;
class ISessionNtfy : public EpollerObj
{
public:

    /**
     *  @brief ��鱨��������, ͬʱ��ȡsessionid��Ϣ
     *  @param pkg ����ָ��
     *  @param len �����ѽ��ճ���
     *  @param session ������sessionid, �������
     *  @return <=0 ʧ��, >0 ʵ�ʱ��ĳ���
     */
    virtual int GetSessionId(void* pkg, int len,  int& session) { return 0;};

    /**
     *  @brief ����socket, �����ɶ��¼�
     *  @return fd�ľ��, <0 ʧ��
     */
    virtual int CreateSocket(){return -1;};

    /**
     *  @brief �ر�socket, ֹͣ�����ɶ��¼�
     */
    virtual void CloseSocket(){};

    /**
     *  @brief �ɶ��¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
     *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
     */
    virtual int InputNotify(){return 0;};
    
    /**
     *  @brief ��д�¼�֪ͨ�ӿ�, ����֪ͨ������ܻ��ƻ�����, ���÷���ֵ����
     *  @return 0 ��fd�ɼ������������¼�; !=0 ��fd�������ص�����
     */
    virtual int OutputNotify(){return 0;};
    
    /**
     *  @brief �쳣֪ͨ�ӿ�
     *  @return ���Է���ֵ, ���������¼�����
     */
    virtual int HangupNotify(){return 0;};

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlAdd(void* args){return 0;};

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlDel(void* args){return 0;};

    /**
     * @brief ���캯����������
     */
    ISessionNtfy(): EpollerObj(0) {
        _proto = MT_UDP;
        _buff_size = 0;
        _msg_buff = NULL;
        TAILQ_INIT(&_write_list);
    }
    virtual ~ISessionNtfy() {   };

    /**
     * @brief ���ñ��δ����proto��Ϣ
     */
    void SetProtoType(MULTI_PROTO proto) {
        _proto = proto;
    };

    /**
     * @brief ��ȡ���δ����proto��Ϣ
     * @return proto type
     */
    MULTI_PROTO GetProtoType() {
        return _proto;
    };
    
    /**
     * @brief ����buff��С, ����ʵ��ʹ�õ�msgbuff����
     * @return  0�ɹ�
     */
    void SetMsgBuffSize(int buff_size) {
        _buff_size = buff_size;
    };

    /**
     * @brief ��ȡԤ�õ�buff��С, ��������, ����65535
     * @return  ����������Ϣbuff��󳤶�
     */
    int GetMsgBuffSize()     {
        return (_buff_size > 0) ? _buff_size : 65535;
    }

    /**
     * @brief ֪ͨ�������ȴ�״̬
     */
    void InsertWriteWait(SessionProxy* proxy);

    /**
     * @brief ֪ͨ����ȡ���ȴ�״̬
     */
    void RemoveWriteWait(SessionProxy* proxy);

    /**
     * @brief �۲���ģʽ, ֪ͨд�ȴ��߳�
     * @info UDP����֪ͨÿ���߳�ִ��д����, TCP��Ҫ�Ŷ�д
     */
    virtual void NotifyWriteWait(){};
    
protected:
    MULTI_PROTO         _proto;         // Э������ UDP/TCP
    int                 _buff_size;     // �����Ϣ����
    NtfyList            _write_list;    // ��д�ȴ�����
    MtMsgBuf*           _msg_buff;      // ��ʱ�հ���Ż�����
};


/**
 * @brief UDP������sessionģ�͵Ļ���ӿ�
 * @info  ҵ��session��Ҫ�̳иýӿ�, ��������, ʵ�ֻ�ȡGetSessionId����
 * @info  ������չ, ��ָ�����ض˿ڵ�
 */
class UdpSessionNtfy : public ISessionNtfy
{
public:
    
    /**
     *  @brief ��鱨��������, ͬʱ��ȡsessionid��Ϣ, �ɼ̳���ʵ����
     *  @param pkg ����ָ��
     *  @param len �����ѽ��ճ���
     *  @param session ������sessionid, �������
     *  @return <=0 ʧ��, >0 ʵ�ʱ��ĳ���
     */
    virtual int GetSessionId(void* pkg, int len,  int& session) { return 0;};


public:

    /**
     * @brief ��������������
     */
    UdpSessionNtfy() : ISessionNtfy(){
        ISessionNtfy::SetProtoType(MT_UDP); 
        
        _local_addr.sin_family = AF_INET;
        _local_addr.sin_addr.s_addr = 0;
        _local_addr.sin_port = 0;
    }
    virtual ~UdpSessionNtfy() {    };

    /**
     * @brief �۲���ģʽ, ֪ͨд�ȴ��߳�
     * @info UDP����֪ͨÿ���߳�ִ��д����, TCP��Ҫ�Ŷ�д
     */
    virtual void NotifyWriteWait();

    /**
     *  @brief ����socket, �����ɶ��¼�
     *  @return fd�ľ��, <0 ʧ��
     */
    virtual int CreateSocket();

    /**
     *  @brief �ر�socket, ֹͣ�����ɶ��¼�
     */
    virtual void CloseSocket();

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

public:

    /**
     * @brief ����udp���صı���bind��ַ, �����bind���ͻ, ��ʱͣ��
     *      ��������, �ܱ�֤ÿ����Ψһport��ʹ��
     */
    void SetLocalAddr(struct sockaddr_in* local_addr) {
        memcpy(&_local_addr, local_addr, sizeof(_local_addr));
    };

protected:

    struct sockaddr_in  _local_addr;
};



/**
 * @brief UDPģʽsessionģ�͵Ĵ���֪ͨ����, �������ӳ�䵽ĳһ��session notify
 * @info  session proxy ������epollע��, �������¼�֪ͨ, ����Ҫ���ĳ�ʱ��
 */
class SessionProxy  : public EpollerObj
{
public:
    int         _flag;                ///< 0-���ڶ�����, 1-�ڵȴ�����
    NtfyEntry   _write_entry;         ///< ������д�ȴ����еĹ������

    /**
     *  @brief ���ô������, ���������fd���
     */
    void SetRealNtfyObj(ISessionNtfy* obj) {
        _real_ntfy = obj;
        this->SetOsfd(obj->GetOsfd());
    };
    
    /**
     *  @brief ��ȡ�������ָ��
     */
    ISessionNtfy* GetRealNtfyObj() {
        return _real_ntfy;
    };

public:

    /**
     * @brief ���մ���, ����������
     */
    virtual void Reset() {
        _real_ntfy = NULL;
        this->EpollerObj::Reset();
    };

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlAdd(void* args) {
        if (!_real_ntfy) {
            return -1;
        }
        
        int events = this->GetEvents(); 
        if (!(events & EPOLLOUT)) {
            return 0;
        }

        if (_real_ntfy->EpollCtlAdd(args) < 0) {
            return -2;
        }
        
        _real_ntfy->InsertWriteWait(this);
        return 0;
    };

    /**
     *  @brief ����epoll�����¼��Ļص��ӿ�, ������ʼ��EPOLLIN, ż��EPOLLOUT
     *  @param args fd���ö����ָ��
     *  @return 0 �ɹ�, < 0 ʧ��, Ҫ������ع�������ǰ״̬
     */
    virtual int EpollCtlDel(void* args) {
        if (!_real_ntfy) {
            return -1;
        } 
        
        int events = this->GetEvents(); 
        if (!(events & EPOLLOUT)) {
            return 0;
        }
        
        _real_ntfy->RemoveWriteWait(this);        
        return _real_ntfy->EpollCtlDel(args);
    };

private:
    ISessionNtfy*   _real_ntfy;         // ʵ�ʵ�ִ����

};

/**
 * @brief TCPģʽ��keepalive֪ͨ����, �������Ŀɶ��¼�, ȷ���Ƿ�Զ˹ر�
 */
class TcpKeepNtfy: public EpollerObj
{
public:

    /**
     * @brief ���캯��
     */
    TcpKeepNtfy() :     _keep_conn(NULL){};    

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
     *  @brief ���ô������
     */
    void SetKeepNtfyObj(TcpKeepConn* obj) {
        _keep_conn = obj;
    };

    /**
     *  @brief ��ȡ�������ָ��
     */
    TcpKeepConn* GetKeepNtfyObj() {
        return _keep_conn;
    };
    
    /**
     *  @brief ����ʵ�����ӹرղ���
     */
    void KeepaliveClose();
    

private:
    TcpKeepConn*   _keep_conn;         // ʵ�ʵ�����������

};


/**
 * @brief ��̬�ڴ��ģ����, ���ڷ���new/delete�Ķ������, ��һ���̶����������
 */
template<typename ValueType>
class CPtrPool
{
public:
    typedef typename std::queue<ValueType*>  PtrQueue; ///< �ڴ�ָ�����
    
public:

    /**
     * @brief ��̬�ڴ�ع��캯��
     * @param max �����ж��б����ָ��Ԫ��, Ĭ��500
     */
    explicit CPtrPool(int max = 500) : _max_free(max), _total(0){};
    
    /**
     * @brief ��̬�ڴ����������, ���������freelist
     */
    ~CPtrPool()    {
        ValueType* ptr = NULL;
        while (!_ptr_list.empty()) {
            ptr = _ptr_list.front();
            _ptr_list.pop();
            delete ptr;
        }
    };

    /**
     * @brief �����ڴ�ָ��, ���ȴӻ����ȡ, �޿��п�����̬ new ����
     * @return ģ�����͵�ָ��Ԫ��, �ձ�ʾ�ڴ�����ʧ��
     */
    ValueType* AllocPtr() {
        ValueType* ptr = NULL;
        if (!_ptr_list.empty()) {
            ptr = _ptr_list.front();
            _ptr_list.pop();
        } else {
            ptr = new ValueType;
            _total++;
        }

        return ptr;
    };

    /**
     * @brief �ͷ��ڴ�ָ��, �����ж��г������, ��ֱ���ͷ�, ������л���
     */
    void FreePtr(ValueType* ptr) {
        if ((int)_ptr_list.size() >= _max_free) {
            delete ptr;
            _total--;
        } else {
            _ptr_list.push(ptr);
        }
    };    
    
protected:
    PtrQueue  _ptr_list;           ///<  ���ж���
    int       _max_free;           ///<  ������Ԫ�� 
    int       _total;              ///<  ����new�Ķ������ͳ��
};


/**
 * @brief ֪ͨ����ȫ�ֹ�����
 */
class NtfyObjMgr
{
public:

    typedef std::map<int, ISessionNtfy*>   SessionMap;
    typedef CPtrPool<EpollerObj> NtfyThreadQueue;
    typedef CPtrPool<SessionProxy>  NtfySessionQueue;
    
    /**
     * @brief �Ự�����ĵ�ȫ�ֹ������ӿ�
     * @return ȫ�־��ָ��
     */
    static NtfyObjMgr* Instance (void);

    /**
     * @brief ����ӿ�
     */
    static void Destroy(void);

    /**
     * @brief ע�᳤����session��Ϣ
     * @param session_name �����ӵı�ʶ, ÿ�����Ӵ���һ��session��װ��ʽ
     * @param session �����Ӷ���ָ��, ������������
     * @return 0 �ɹ�, < 0 ʧ��
     */
    int RegisterSession(int session_name, ISessionNtfy* session);

    /**
     * @brief ��ȡע�᳤����session��Ϣ
     * @param session_name �����ӵı�ʶ, ÿ�����Ӵ���һ��session��װ��ʽ
     * @return ������ָ��, ʧ��ΪNULL
     */
    ISessionNtfy* GetNameSession(int session_name);

    /**
     * @brief ��ȡͨ��֪ͨ����, ���߳�֪ͨ������session֪ͨ�������
     * @param type ����, �߳�֪ͨ���ͣ�UDP/TCP SESSION֪ͨ��
     * @param session_name proxyģ��,һ����ȡsession����
     * @return ֪ͨ�����ָ��, ʧ��ΪNULL
     */
    EpollerObj* GetNtfyObj(int type, int session_name = 0);

    
    /**
     * @brief �ͷ�֪ͨ����ָ��
     * @param obj ֪ͨ����
     */
    void FreeNtfyObj(EpollerObj* obj);

    /**
     * @brief ��������
     */
    ~NtfyObjMgr();
    
private:

    /**
     * @brief ��Ϣbuff�Ĺ��캯��
     */
    NtfyObjMgr();

    static NtfyObjMgr * _instance;         ///<  ��������
    SessionMap _session_map;               ///<  ȫ�ֵ�ע��session����
    NtfyThreadQueue  _fd_ntfy_pool;        ///<  fd֪ͨ����
    NtfySessionQueue _udp_proxy_pool;      ///<  fd֪ͨ����
};



}

#endif


