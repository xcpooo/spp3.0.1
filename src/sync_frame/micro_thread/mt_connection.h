/**
 *  @file mt_connection.h
 *  @info ΢�߳����ӹ����岿��
 *  @time 20130924
 **/

#ifndef __MT_CONNECTION_H__
#define __MT_CONNECTION_H__

#include <netinet/in.h>
#include <queue>
#include "mt_mbuf_pool.h"
#include "hash_list.h"
#include "mt_action.h"

namespace NS_MICRO_THREAD {

using std::queue;

/**
 * @brief ���Ӷ�������
 */
enum CONN_OBJ_TYPE
{
    OBJ_CONN_UNDEF     = 0,     ///< δ��������Ӷ���
    OBJ_SHORT_CONN     = 1,     ///< �����Ӷ���, fd�����Ự, ÿ������CLOSE
    OBJ_TCP_KEEP       = 2,     ///< TCP�ĸ���ģ��, ÿ��ÿ����ʹ�ø�fd, ����ɸ���
    OBJ_UDP_SESSION    = 3,     ///< UDP��sessionģ��, ÿ���ӿɹ������߳�ʹ��
};

/**
 * @brief ΢�߳�һ���������, ӳ��һ�����Ӷ���
 */
class IMtConnection
{
public:

    /**
     * @brief  ΢�߳����ӻ��๹��������
     */
    IMtConnection();
    virtual ~IMtConnection();

    /**
     * @brief ���ӻ��ո����������
     */
    virtual void Reset();
    
    /**
     * @brief ��ȡ���Ӷ����������Ϣ
     */
    CONN_OBJ_TYPE GetConnType() {
        return _type;    
    };
    
    /**
     * @brief �����ڲ�ACTIONָ��
     * @return IMtConnָ��
     */
    void SetIMtActon(IMtAction* action  ) {
        _action = action;
    };

    /**
     * @brief ��ȡ�ڲ�ACTIONָ��
     * @return IMtConnָ��
     */
    IMtAction* GetIMtActon() {
        return _action;
    };

    /**
     * @brief �����ڲ�ACTIONָ��
     * @return IMtConnָ��
     */
    void SetNtfyObj(EpollerObj* obj  ) {
        _ntfy_obj = obj;
    };

    /**
     * @brief ��ȡ�ڲ�ACTIONָ��
     * @return IMtConnָ��
     */
    EpollerObj* GetNtfyObj() {
        return _ntfy_obj;
    };
    
    /**
     * @brief �����ڲ�msgbuffָ��
     * @return IMtConnָ��
     */
    void SetMtMsgBuff(MtMsgBuf* msg_buf) {
        _msg_buff = msg_buf;
    };

    /**
     * @brief ��ȡ�ڲ�msgbuffָ��
     * @return IMtConnָ��
     */
    MtMsgBuf* GetMtMsgBuff() {
        return _msg_buff;
    };   

public:
    
    /**
     * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CreateSocket() {return 0;};
    
    /**
     * @brief  ���Ӵ���Զ�˻Ựͨ��, ��TCP��connect��
     * @return 0 -�ɹ�, < 0 ʧ�� 
     */
    virtual int OpenCnnect() {return 0;};

    /**
     * @brief  ���ӷ�������
     * @return >0 -�ɹ�, ����ʵ�ʷ��ͳ���, < 0 ʧ�� 
     */
    virtual int SendData() {return 0;};

    /**
     * @brief  ���ӽ�������
     * @return >0 -�ɹ�, ���ر��ν��ճ���, < 0 ʧ��(-1 �Զ˹ر�; -2 ���մ���)
     */
    virtual int RecvData() {return 0;};

    /**
     * @brief  �ر�socket�˿�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CloseSocket() {return 0;};

protected:

    CONN_OBJ_TYPE       _type;      // Ԥ�õ�type, �ɰ�type����������
    IMtAction*          _action;    // ������actionָ��, �ϼ�ָ��, ��������Դ������
    EpollerObj*         _ntfy_obj;  // EPOLL֪ͨ����, �¼�ָ��, ����������
    MtMsgBuf*           _msg_buff;  // ��̬�����buff�ֶ�, �¼�ָ��, ����������
};

/**
 * @brief ����sock�Ķ���������
 */
class UdpShortConn : public IMtConnection
{
public:

    /**
     * @brief ����socket�Ķ����ӵĹ���������
     */
    UdpShortConn() {
        _osfd = -1;
        _type = OBJ_SHORT_CONN;
    };    
    virtual ~UdpShortConn() {
        CloseSocket();
    };

    /**
     * @brief ���ӻ��ո����������
     */
    virtual void Reset();

    /**
     * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CreateSocket();

    /**
     * @brief  ���ӷ�������
     * @return >0 -�ɹ�, ����ʵ�ʷ��ͳ���, < 0 ʧ�� 
     */
    virtual int SendData();

    /**
     * @brief  ���ӽ�������
     * @return >0 -�ɹ�, ���ر��ν��ճ���, < 0 ʧ��(-1 �Զ˹ر�; -2 ���մ���)
     */
    virtual int RecvData();

    /**
     * @brief  �ر�socket�˿�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CloseSocket();
    
protected:
    int                 _osfd;      // ÿ�����ӵ�������socket
};


enum TcpKeepFlag
{
    TCP_KEEP_IN_LIST   = 0x1,
    TCP_KEEP_IN_EPOLL  = 0x2,
};

/**
 * @brief ����session��UDP��������
 */
class UdpSessionConn : public IMtConnection
{
public:

    /**
     * @brief ����socket�Ķ����ӵĹ���������
     */
    UdpSessionConn() {
        _type = OBJ_UDP_SESSION;
    };    
    virtual ~UdpSessionConn() {    };

    /**
     * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CreateSocket();

    /**
     * @brief  ���ӷ�������
     * @return >0 -�ɹ�, ����ʵ�ʷ��ͳ���, < 0 ʧ�� 
     */
    virtual int SendData();

    /**
     * @brief  ���ӽ�������
     * @return >0 -�ɹ�, ���ر��ν��ճ���, < 0 ʧ��(-1 �Զ˹ر�; -2 ���մ���)
     */
    virtual int RecvData();

    /**
     * @brief  �ر�socket�˿�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CloseSocket();
};

/**
 * @brief ����sock��TCP��������
 */
typedef TAILQ_ENTRY(TcpKeepConn) KeepConnLink;
typedef TAILQ_HEAD(__KeepConnTailq, TcpKeepConn) KeepConnList;
class TcpKeepConn : public IMtConnection, public CTimerNotify
{
public:

    int           _keep_flag;  // ����״̬���
    KeepConnLink  _keep_entry; // ���й������

    /**
     * @brief ����socket�Ķ����ӵĹ���������
     */
    TcpKeepConn() {
        _osfd = -1;
        _keep_time = 10*60*1000; // Ĭ��10����, ���԰������
        _keep_flag = 0;
        _type = OBJ_TCP_KEEP;
        _keep_ntfy.SetKeepNtfyObj(this);
    };    
    virtual ~TcpKeepConn() {
        CloseSocket();
    };

    /**
     * @brief ���ӻ��ո����������
     */
    virtual void Reset();
    
    /**
     * @brief  ���Ӵ���Զ�˻Ựͨ��, ��TCP��connect��
     * @return 0 -�ɹ�, < 0 ʧ�� 
     */
    virtual int OpenCnnect();

    /**
     * @brief  ���ӵ�socket����, �������ӵ�Э�����͵�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CreateSocket();

    /**
     * @brief  ���ӷ�������
     * @return >0 -�ɹ�, ����ʵ�ʷ��ͳ���, < 0 ʧ�� 
     */
    virtual int SendData();

    /**
     * @brief  ���ӽ�������
     * @return >0 -�ɹ�, ���ر��ν��ճ���, < 0 ʧ��(-1 �Զ˹ر�; -2 ���մ���)
     */
    virtual int RecvData();

    /**
     * @brief  �ر�socket�˿�
     * @return >0 -�ɹ�, ����ϵͳfd, < 0 ʧ�� 
     */
    virtual int CloseSocket();

    /**
     * @brief ���ӱ��ָ���
     */
    void ConnReuseClean();

    /**
     * @brief Idle���洦��, epoll ����Զ�˹رյ�
     */
    bool IdleAttach();

    /**
     * @brief Idleȡ�����洦��, �����ɿ����߳�����Զ�˹ر�
     */
    bool IdleDetach();

    /**
     * @brief �洢Ŀ�ĵ�ַ��Ϣ, ���ڸ���
     */
    void SetDestAddr(struct sockaddr_in* dst) {
        memcpy(&_dst_addr, dst, sizeof(_dst_addr));
    }

    /**
     * @brief ��ȡĿ�ĵ�ַ��Ϣ
     */
    struct sockaddr_in* GetDestAddr() {
        return &_dst_addr;
    }

    /**
     * @brief ��ʱ֪ͨ����, ����ʵ���߼�
     */
    virtual void timer_notify();

    /**
     * @brief ���ó�ʱʱ��, ���뵥λ
     */
    void SetKeepTime(unsigned int time) {
        _keep_time = time;    
    };
    
protected:
    int                 _osfd;      // ÿ�����ӵ�������socket
    unsigned int        _keep_time; // ���ñ����ʱ��
    TcpKeepNtfy         _keep_ntfy; // ����һ���������Ӷ���
    struct sockaddr_in  _dst_addr;  // Զ�˵�ַ��Ϣ
    
};



/**
 * @brief ����ַhash���泤����
 */
class TcpKeepKey : public HashKey
{
public:

    /**
     * @brief ��������������
     */
    TcpKeepKey() {
        _addr_ipv4  = 0;
        _net_port   = 0;
        TAILQ_INIT(&_keep_list);
        this->SetDataPtr(this);
    };

    TcpKeepKey(struct sockaddr_in * dst) {
        _addr_ipv4  = dst->sin_addr.s_addr;
        _net_port   = dst->sin_port;
        TAILQ_INIT(&_keep_list);
        this->SetDataPtr(this);
    };

    /**
     * @brief �����ݲ�����conn
     */
    ~TcpKeepKey() {
        TAILQ_INIT(&_keep_list);
    };

    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue(){
        return _addr_ipv4 ^ ((_net_port << 16) | _net_port);
    }; 

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs){
        TcpKeepKey* data = dynamic_cast<TcpKeepKey*>(rhs);
        if (!data) { 
            return -1;
        }
        if (this->_addr_ipv4 != data->_addr_ipv4) {
            return this->_addr_ipv4 - data->_addr_ipv4;    
        }
        if (this->_net_port != data->_net_port) {
            return this->_net_port - data->_net_port;
        }
        return 0;
    }; 


    /**
     * @brief ���Ӷ������
     */
    void InsertConn(TcpKeepConn* conn) {
        if (conn->_keep_flag & TCP_KEEP_IN_LIST) {
            return;
        }
        TAILQ_INSERT_TAIL(&_keep_list, conn, _keep_entry);
        conn->_keep_flag |= TCP_KEEP_IN_LIST;
    };
    
    void RemoveConn(TcpKeepConn* conn) {
        if (!(conn->_keep_flag & TCP_KEEP_IN_LIST)) {
            return;
        }
        TAILQ_REMOVE(&_keep_list, conn, _keep_entry);
        conn->_keep_flag &= ~TCP_KEEP_IN_LIST;
    };

    TcpKeepConn* GetFirstConn() {
        return TAILQ_FIRST(&_keep_list);
    };    

private:
    uint32_t            _addr_ipv4;     ///< ip��ַ
    uint16_t            _net_port;      ///< port ��������
    KeepConnList        _keep_list;     ///< ʵ�ʵĿ��ж���
    
};


/**
 * @brief TCP�����ӵ����Ӷ���������ڴ�cache
 */
class TcpKeepMgr
{
public:

    typedef CPtrPool<TcpKeepConn>   TcpKeepQueue;   ///< �ڴ滺���

    /**
     * @brief ��������������
     */
    TcpKeepMgr();

    ~TcpKeepMgr();


    /**
     * @brief ��IP��ַ��ȡTCP�ı�������
     */
    TcpKeepConn* GetTcpKeepConn(struct sockaddr_in*       dst);
    
    /**
     * @brief ��IP��ַ����TCP�ı�������
     */
    bool CacheTcpKeepConn(TcpKeepConn* conn);    

    /**
     * @brief ��IP��ַ����TCP�ı�������, ȥ��CACHE
     */
    bool RemoveTcpKeepConn(TcpKeepConn* conn); 

    /**
     * @brief �رջ򻺴�tcp������
     */
    void FreeTcpKeepConn(TcpKeepConn* conn, bool force_free);    
    
private:

    HashList*       _keep_hash;            ///< hash��, �洢��IP���������Ӷ���
    TcpKeepQueue    _mem_queue;            ///< mem����, ����conn�ڴ��
};


/**
 * @brief ���ӹ�����ģ��
 */
class ConnectionMgr
{
public:

    typedef CPtrPool<UdpShortConn>      UdpShortQueue;
    typedef CPtrPool<UdpSessionConn>    UdpSessionQueue;

    /**
     * @brief ��Ϣbuff��ȫ�ֹ������ӿ�
     * @return ȫ�־��ָ��
     */
    static ConnectionMgr* Instance (void);

    /**
     * @brief ��Ϣ����ӿ�
     */
    static void Destroy(void);

    /**
     * @brief ��ȡ�ӿ�
     */
    IMtConnection* GetConnection(CONN_OBJ_TYPE type, struct sockaddr_in*     dst);
    
    /**
     * @brief ���սӿ�
     */
    void FreeConnection(IMtConnection* conn, bool force_free);

    /**
     * @brief �ر�idle��tcp������
     */
    void CloseIdleTcpKeep(TcpKeepConn* conn);

    /**
     * @brief ��Ϣbuff����������
     */
    ~ConnectionMgr();

private:
    /**
     * @brief ��Ϣbuff�Ĺ��캯��
     */
    ConnectionMgr();

    static ConnectionMgr * _instance;         ///< �������� 

    UdpShortQueue  _udp_short_queue;          ///< �����ӵĶ��г� 
    UdpSessionQueue  _udp_session_queue;      ///< udp session ���ӳ�
    TcpKeepMgr      _tcp_keep_mgr;            ///< tcp keep ������
};

}
#endif


