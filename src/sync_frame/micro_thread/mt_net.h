/**
 *  @file mt_net.h
 *  @info ΢�̷߳�װ������ӿ���
 **/

#ifndef __MT_NET_H__
#define __MT_NET_H__

#include "micro_thread.h"
#include "hash_list.h"
#include "mt_api.h"
#include "mt_cache.h"
#include "mt_net_api.h"

namespace NS_MICRO_THREAD {

/**
 * @brief �������Ͷ���
 */
enum MT_CONN_TYPE 
{
    TYPE_CONN_UNKNOWN   = 0,
    TYPE_CONN_SHORT     = 0x1,          ///< ������, һ�ν�����ر�
    TYPE_CONN_POOL      = 0x2,          ///< �����ӣ�ÿ��ʹ�ú�, �ɻ����ظ�ʹ��
    TYPE_CONN_SESSION   = 0x4,          ///< �����ӣ���session id ����, ������
    TYPE_CONN_SENDONLY  = 0x8,          ///< ֻ������
};


/******************************************************************************/
/*  �ڲ�ʵ�ֲ���                                                              */
/******************************************************************************/
class CSockLink;

/**
 * @brief ��ʱ���յĶ����ģ��ʵ��
 * @info  List������tailq, Type ��Ҫ��reset����, releasetime, linkentry�ֶ�
 */
template <typename List, typename Type>
class CRecyclePool
{
public:

    // ���캯��, Ĭ��60s��ʱ
    CRecyclePool() {
        _expired = 60 * 1000;
        _count = 0;
        TAILQ_INIT(&_free_list);
    };

    // ��������, ɾ������Ԫ��
    ~CRecyclePool() {
        Type* item = NULL;
        Type* tmp = NULL;
        TAILQ_FOREACH_SAFE(item, &_free_list, _link_entry, tmp)
        {
            TAILQ_REMOVE(&_free_list, item, _link_entry);
            delete item;
        }
        _count = 0;
    };

    // ���û��´�������
    Type* AllocItem() {
        Type* item = TAILQ_FIRST(&_free_list);
        if (item != NULL)
        {
            TAILQ_REMOVE(&_free_list, item, _link_entry);
            _count--;
            return item;
        }
        
        item = new Type();
        if (NULL == item)
        {
            return NULL;
        }
        
        return item;
    };

    // �ͷŹ������
    void FreeItem(Type* obj) {
        //obj->Reset();        
        TAILQ_INSERT_TAIL(&_free_list, obj, _link_entry);
        obj->_release_time = mt_time_ms();
        _count++;
    };
    

    // ���վ��
    void RecycleItem(uint64_t now) {
        Type* item = NULL;
        Type* tmp = NULL;
        TAILQ_FOREACH_SAFE(item, &_free_list, _link_entry, tmp)
        {
            if ((now - item->_release_time) < _expired) {
                break;
            }
        
            TAILQ_REMOVE(&_free_list, item, _link_entry);
            delete item;
            _count--;
        }
    };

    // �����Զ���ĳ�ʱʱ��
    void SetExpiredTime(uint64_t expired) {
        _expired = expired;
    };

private:

    List            _free_list;      ///< ��������
    uint64_t        _expired;        ///< ��ʱʱ��
    uint32_t        _count;          ///< Ԫ�ؼ���
};



/**
 * @brief ÿ��IO����һ���������
 */
class CNetHandler : public HashKey
{
public:

    // ���״̬����
    enum {
        STATE_IN_SESSION    = 0x1,
        STATE_IN_CONNECT    = 0x2,
        STATE_IN_SEND       = 0x4,
        STATE_IN_RECV       = 0x8,
        STATE_IN_IDLE       = 0x10,
    };
    
    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue();

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs); 

    // ͬ���շ��ӿ�
    int32_t SendRecv(void* data, uint32_t len, uint32_t timeout);

    // ��ȡ����buff��Ϣ, ��Ч��ֱ��helper����
    void* GetRspBuff() {
        if (_rsp_buff != NULL) {
            return _rsp_buff->data;
        } else {
            return NULL;
        }
    };

    // ��ȡ����buff��Ϣ, ��Ч��ֱ��helper����
    uint32_t GetRspLen() {
        if (_rsp_buff != NULL) {
            return _rsp_buff->data_len;
        } else {
            return 0;
        }
    };
    
    // ����rsp��Ϣ
    void SetRespBuff(TSkBuffer* buff) {
        if (_rsp_buff != NULL) {
            delete_sk_buffer(_rsp_buff);
            _rsp_buff = NULL;
        }
        
        _rsp_buff = buff;
    };

    // ����Э�������, Ĭ��UDP
    void SetProtoType(MT_PROTO_TYPE type) {
        _proto_type = type;    
    };

    // ������������, Ĭ�ϳ�����
    void SetConnType(MT_CONN_TYPE type) {
        _conn_type = type;
    };

    // ����Ŀ��IP��ַ
	void SetDestAddress(struct sockaddr_in* dst) {
        if (dst != NULL) {
            memcpy(&_dest_ipv4, dst, sizeof(*dst));
        }
	};

	// ����session����session id��Ϣ, �����0
	void SetSessionId(uint64_t sid) {
        _session_id = sid;
	};	

    // ����session�����ص�����
    void SetSessionCallback(CHECK_SESSION_CALLBACK function) {
        _callback = function;
    };

    // ��ȡ�ص�������Ϣ
    CHECK_SESSION_CALLBACK GetSessionCallback() {
        return _callback;
    };
    

public:

    // �������Ӷ���
    void Link(CSockLink* conn);

    // �������Ӷ���
    void Unlink();
    
    // ����Ҫ�Ĳ�����Ϣ 
    int32_t CheckParams();

    // ��ȡ����, ͬʱ�������ȴ����ӵĶ����� 
    int32_t GetConnLink();

    // ����Ҫ�Ĳ�����Ϣ 
    int32_t WaitConnect(uint64_t timeout);

    // ����Ҫ�Ĳ�����Ϣ 
    int32_t WaitSend(uint64_t timeout);

    // ����Ҫ�Ĳ�����Ϣ 
    int32_t WaitRecv(uint64_t timeout);

    // �����ڵȴ����Ӷ���
    void SwitchToConn();

    // �л������Ͷ���
    void SwitchToSend();

    // �л������ն���
    void SwitchToRecv();
    
    // �л�������״̬
    void SwitchToIdle();

    // �������Ӷ���
    void DetachConn();
    
    // ע��session����
    bool RegistSession();

    // ȡ��ע��session
    void UnRegistSession();

    // �������͵����󳤶�
    uint32_t SkipSendPos(uint32_t len);

    // ���÷�����
    void SetErrNo(int32_t err) {
        _err_no = err;
    };

    // ��ȡ�������߳���Ϣ
    MicroThread* GetThread() {
        return _thread;
    };

    // ��ȡ�����͵�ָ�������ݳ���
    void GetSendData(void*& data, uint32_t& len) {
        data = _req_data;
        len  = _req_len;
    };
 
    // ���ýӿ�
    void Reset();

    // ����������
    CNetHandler();
    ~CNetHandler();

    // ���п�ݷ��ʵĺ궨��
    TAILQ_ENTRY(CNetHandler)    _link_entry; 
    uint64_t                    _release_time;

protected:

    MicroThread*        _thread;            ///< �����߳�ָ�����
    MT_PROTO_TYPE       _proto_type;        ///< Э������       
    MT_CONN_TYPE        _conn_type;         ///< ��������
    struct sockaddr_in  _dest_ipv4;         ///< ipv4Ŀ�ĵ�ַ
    uint64_t            _session_id;        ///< �ỰID
    CHECK_SESSION_CALLBACK _callback;       ///< �Ự��ȡ�ص�����
    uint32_t            _state_flags;       ///< �ڲ�״̬�ֶ�
    int32_t             _err_no;            ///< ��������Ϣ
    void*               _conn_ptr;          ///< socket ��·ָ��
    uint32_t            _send_pos;          ///< �ѷ��͵�posλ��
    uint32_t            _req_len;           ///< ���������
    void*               _req_data;          ///< �����ָ��
    TSkBuffer*          _rsp_buff;          ///< Ӧ��buff��Ϣ

};
typedef TAILQ_HEAD(__NetHandlerList, CNetHandler) TNetItemList;  ///< ��Ч��˫������ 
typedef CRecyclePool<TNetItemList, CNetHandler>   TNetItemPool;   ///< ��ʱ���յĶ����


/**
 * @brief ��������·����
 */
class CSockLink : public EpollerObj
{
public:

    // ���״̬����
    enum {
        LINK_CONNECTING     = 0x1,
        LINK_CONNECTED      = 0x2,
    };

    // ״̬���ж���
    enum {
        LINK_IDLE_LIST      = 1,
        LINK_CONN_LIST      = 2,
        LINK_SEND_LIST      = 3,
        LINK_RECV_LIST      = 4,
    };

    // ���򴴽�socket���
    int32_t CreateSock();

    // �ر���·�ľ��
    void Close();

    // �������ӹ���
    bool Connect();
    bool Connected() {
        return (_state & LINK_CONNECTED);
    }

    // �쳣��ֹ�Ĵ�����
    void Destroy();  

    // ��ȡ��������
    TNetItemList* GetItemList(int32_t type);

    // ��������Ϣ
    void AppendToList(int32_t type, CNetHandler* item);

    // ��������Ϣ
    void RemoveFromList(int32_t type, CNetHandler* item);

    // ��ȡĿ��ip��Ϣ
    struct sockaddr_in* GetDestAddr(struct sockaddr_in* addr);

    // �������ӹ���
    int32_t SendData(void* data, uint32_t len);

    // udp��������
    int32_t SendCacheUdp(void* data, uint32_t len);

    // tcp��������
    int32_t SendCacheTcp(void* data, uint32_t len);

    // ���Խ��ո�������ݵ���ʱbuff
    void ExtendRecvRsp();

    // ���ݷַ��������
    int32_t RecvDispath();

    // ���߻ص�����, ���ȴ��Ŷӵȴ��л�ȡ, ���ݴӸ��ڵ��ȡ
    CHECK_SESSION_CALLBACK GetSessionCallback();

    // TCP����������������ַ�
    int32_t DispathTcp();

    // UDP����������������ַ�
    int32_t DispathUdp();

    // ��ѯ����sessionid������session��Ϣ
    CNetHandler* FindSession(uint64_t sid);

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


    // ��������������
    CSockLink();
    ~CSockLink();    
    
    // �����ó�ʼ���߼�
    void Reset();
    
    // ֪ͨ�����߳�
    void NotifyThread(CNetHandler* item, int32_t result);

    // ֪ͨ�����߳�
    void NotifyAll(int32_t result);

    // ����Э������, ����buff�ص�ָ��
    void SetProtoType(MT_PROTO_TYPE type);

    // �����ϼ�ָ����Ϣ
    void SetParentsPtr(void* ptr) {
        _parents = ptr;
    };

    // ��ȡ�ϼ��ڵ�ָ��
    void* GetParentsPtr() {
        return _parents;
    };

    // ��ȡ�ϴεķ���ʱ��
    uint64_t GetLastAccess() {
        return _last_access;
    };
    
    

public:

    // ���п�ݷ��ʵĺ궨��
    TAILQ_ENTRY(CSockLink) _link_entry; 
    uint64_t               _release_time;
    
private:

    TNetItemList        _wait_connect;
    TNetItemList        _wait_send;
    TNetItemList        _wait_recv;
    TNetItemList        _idle_list;
    MT_PROTO_TYPE       _proto_type;
    int32_t             _errno;
    uint32_t            _state;
    uint64_t            _last_access;
    TRWCache            _recv_cache;
    TSkBuffer*          _rsp_buff;
    void*               _parents;
};
typedef TAILQ_HEAD(__SocklinkList, CSockLink) TLinkList;  ///< ��Ч��˫������ 
typedef CRecyclePool<TLinkList, CSockLink>    TLinkPool;   ///< ��ʱ���յĶ����


class CDestLinks : public CTimerNotify, public HashKey
{
public:

    // ���캯��
    CDestLinks();

    // ��������
    ~CDestLinks();

    // ���ø��õĽӿں���
    void Reset();

    // ������ʱ��
    void StartTimer();

    // ��ȡһ������link, ��ʱ����ѯ
    CSockLink* GetSockLink();

    // �ͷ�һ������link
    void FreeSockLink(CSockLink* sock);

    // ��ȡЭ������
    MT_PROTO_TYPE GetProtoType() {
        return _proto_type;
    };

    // ��ȡ��������
    MT_CONN_TYPE GetConnType() {
        return _conn_type;
    };
    

    // ���ùؼ���Ϣ
    void SetKeyInfo(uint32_t ipv4, uint16_t port, MT_PROTO_TYPE proto, MT_CONN_TYPE conn) {
        _addr_ipv4  = ipv4;
        _net_port   = port;
        _proto_type = proto;
        _conn_type  = conn;
    };

    // ����KEY��Ϣ
    void CopyKeyInfo(CDestLinks* key) {
        _addr_ipv4  = key->_addr_ipv4;
        _net_port   = key->_net_port;
        _proto_type = key->_proto_type;
        _conn_type  = key->_conn_type;
    };
    
    // ��ȡIP port��Ϣ
    void GetDestIP(uint32_t& ip, uint16_t& port) {
        ip = _addr_ipv4;
        port = _net_port;
    };

    /**
     * @brief ��ʱ֪ͨ����, ��������·, ���������·����
     */
    virtual void timer_notify();
    
    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue() {
        return _addr_ipv4 ^ (((uint32_t)_net_port << 16) | (_proto_type << 8) | _conn_type);
    }; 

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs) {
        CDestLinks* data = (CDestLinks*)(rhs);
        if (!data) { 
            return -1;
        }
        if (this->_addr_ipv4 != data->_addr_ipv4) {
            return (this->_addr_ipv4 > data->_addr_ipv4) ?  1 : -1;    
        }
        if (this->_net_port != data->_net_port) {
            return (this->_net_port > data->_net_port) ? 1 : -1;
        }
        if (this->_proto_type != data->_proto_type) {
            return (this->_proto_type > data->_proto_type) ? 1 : -1;
        }
        if (this->_conn_type != data->_conn_type) {
            return (this->_conn_type > data->_conn_type) ? 1 : -1;
        }
        
        return 0;
    }; 

    // ����session�����ص�����
    void SetDefaultCallback(CHECK_SESSION_CALLBACK function) {
        _dflt_callback = function;
    };

    // ��ȡ�ص�������Ϣ
    CHECK_SESSION_CALLBACK GetDefaultCallback() {
        return _dflt_callback;
    };

    // ���п�ݷ��ʵĺ궨��
    TAILQ_ENTRY(CDestLinks) _link_entry; 
    uint64_t                _release_time;

private:

    uint32_t            _timeout;       ///< idle�ĳ�ʱʱ��
    uint32_t            _addr_ipv4;     ///< ip��ַ
    uint16_t            _net_port;      ///< port ��������
    MT_PROTO_TYPE       _proto_type;    ///< Э������
    MT_CONN_TYPE        _conn_type;     ///< ��������

    uint32_t            _max_links;     ///< ���������
    uint32_t            _curr_link;     ///< ��ǰ������
    TLinkList           _sock_list;     ///< ��������
    CHECK_SESSION_CALLBACK _dflt_callback; ///< Ĭ�ϵ�check����
        
};
typedef TAILQ_HEAD(__DestlinkList, CDestLinks) TDestList;  ///< ��Ч��˫������ 
typedef CRecyclePool<TDestList, CDestLinks>    TDestPool;   ///< ��ʱ���յĶ����

/**
 * @brief ���ӹ�����ģ��
 */
class CNetMgr
{
public:

    /**
     * @brief ��Ϣbuff��ȫ�ֹ������ӿ�
     * @return ȫ�־��ָ��
     */
    static CNetMgr* Instance (void);

    /**
     * @brief ��Ϣ����ӿ�
     */
    static void Destroy(void);

    // ��ѯ�Ƿ��Ѿ�����ͬһ��sid�Ķ���
    CNetHandler* FindNetItem(CNetHandler* key);

    // ע��һ��item, �Ȳ�ѯ�����, ��֤�޳�ͻ
    void InsertNetItem(CNetHandler* item);

    // �Ƴ�һ��item����
    void RemoveNetItem(CNetHandler* item);

    // ��ѯ�򴴽�һ��Ŀ��ip��links�ڵ�
    CDestLinks* FindCreateDest(CDestLinks* key);

    // ɾ�������е�Ŀ����·��Ϣ
    void DeleteDestLink(CDestLinks* dst);
    
    // ��ѯ�Ƿ��Ѿ�����ͬһ��sid�Ķ���
    CDestLinks* FindDestLink(CDestLinks* key);

    // ע��һ��item, �Ȳ�ѯ�����, ��֤�޳�ͻ
    void InsertDestLink(CDestLinks* item);
    
    // �Ƴ�һ��item����
    void RemoveDestLink(CDestLinks* item);

    /**
     * @brief ��Ϣbuff����������
     */
    ~CNetMgr();

    /**
     * @brief ������Դ��Ϣ
     */
    void RecycleObjs(uint64_t now);

    // ����һ�����������
    CNetHandler* AllocNetItem() {
        return _net_item_pool.AllocItem();
    };

    // �ͷ�һ�����������
    void FreeNetItem(CNetHandler* item) {
        return _net_item_pool.FreeItem(item);
    };

    // ����һ��SOCK������·
    CSockLink* AllocSockLink() {
        return _sock_link_pool.AllocItem();
    };

    // �ͷ�һ��SOCK������·
    void FreeSockLink(CSockLink* item) {
        return _sock_link_pool.FreeItem(item);
    };

    // ����һ��SOCK������·
    CDestLinks* AllocDestLink() {
        return _dest_ip_pool.AllocItem();
    };

    // �ͷ�һ��SOCK������·
    void FreeDestLink(CDestLinks* item) {
        return _dest_ip_pool.FreeItem(item);
    };

    // ��ȡudp��buff����Ϣ
    TSkBuffMng* GetSkBuffMng(MT_PROTO_TYPE type) {
        if (type == NET_PROTO_TCP) {
            return &_tcp_pool;
        } else {
            return &_udp_pool;
        }
    };
    

private:
    /**
     * @brief ��Ϣbuff�Ĺ��캯��
     */
    CNetMgr();

    static CNetMgr *    _instance;          ///< �������� 
    HashList*           _ip_hash;           ///< Ŀ�ĵ�ַhash
    HashList*           _session_hash;      ///< session id��hash
    TSkBuffMng          _udp_pool;          ///< udp pool, 64K
    TSkBuffMng          _tcp_pool;          ///< tcp pool, 4K
    TDestPool           _dest_ip_pool;      ///< Ŀ��ip�����
    TLinkPool           _sock_link_pool;    ///< socket pool
    TNetItemPool        _net_item_pool;     ///< net handle pool
};





}

#endif


