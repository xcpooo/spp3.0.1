/**
 *  @file mt_action.h
 *  @info ΢�߳�ACTION���ඨ��
 **/

#ifndef __MT_ACTION_H__
#define __MT_ACTION_H__

#include <netinet/in.h>
#include <queue>
#include "mt_msg.h"
#include "mt_session.h"
#include "mt_notify.h"

namespace NS_MICRO_THREAD {


/**
 * @brief ��������״̬��Ƕ���
 */
enum MULTI_STATE 
{
    MULTI_FLAG_UNDEF   = 0x0,       ///< ��ʼ��, δ����
    MULTI_FLAG_INIT    = 0x1,       ///< socket�����ѳɹ�
    MULTI_FLAG_OPEN    = 0x2,       ///< socket�����Ѵ�
    MULTI_FLAG_SEND    = 0x4,       ///< �������Ѿ�����
    MULTI_FLAG_FIN     = 0x8,       ///< Ӧ�����Ѿ����յ�
};

/**
 * @brief Э���������Ͷ���
 */
enum MULTI_CONNECT 
{
    CONN_UNKNOWN        = 0,
    CONN_TYPE_SHORT     = 0x1,          ///< ������, һ�ν�����ر�
    CONN_TYPE_LONG      = 0x2,          ///< �����ӣ�ÿ��ʹ�ú�, �ɻ����ظ�ʹ��
    CONN_TYPE_SESSION   = 0x4,          ///< �����ӣ���session id ����, ������
};

/**
 * @brief �����붨��
 */
enum MULTI_ERROR 
{
    ERR_NONE            =  0,          
    ERR_SOCKET_FAIL     = -1,          ///< ����sockʧ��
    ERR_CONNECT_FAIL    = -2,          ///< ����ʧ��
    ERR_SEND_FAIL       = -3,          ///< ���ͱ���ʧ��
    ERR_RECV_FAIL       = -4,          ///< ����ʧ��
    ERR_RECV_TIMEOUT    = -5,          ///< ���ճ�ʱ
    ERR_EPOLL_FAIL      = -6,          ///< epollʧ��
    ERR_FRAME_ERROR     = -7,          ///< ���ʧ��
    ERR_PEER_CLOSE      = -8,          ///< �Է��ر� 
    ERR_PARAM_ERROR     = -9,          ///< ��������  
    ERR_MEMORY_ERROR    = -10,         ///< �ڴ�����ʧ��
    ERR_ENCODE_ERROR    = -11,         ///< ���ʧ��
    ERR_DST_ADDR_ERROR  = -12,         ///< Ŀ���ַ��ȡʧ��
};




/**
 * @brief  ΢�̵߳ĺ�˽����������
 */
class IMtAction : public ISession
{
public:

    /**
     * @brief ΢�̲߳�����Ϊ����
     */
    IMtAction();
    virtual ~IMtAction();

	/**
	 * @brief ������������Ϣ (��֤�ӿ����������, ��ʹ��inline)
     * @param  dst -����������͵ĵ�ַ
	 */
	void SetMsgDstAddr(struct sockaddr_in* dst) {
        memcpy(&_addr, dst, sizeof(_addr));
	};
	
	/**
	 * @brief ��ȡ��ϢĿ�ĵ�ַ��Ϣ
     * @return  ע���Ŀ�ĵ�ַ
	 */
	struct sockaddr_in* GetMsgDstAddr() {
        return &_addr;
	};

    /**
     * @brief ����buff��С, ����ʵ��ʹ�õ�msgbuff����
     * @return  0�ɹ�
     */
    void SetMsgBuffSize(int buff_size) {
        _buff_size = buff_size;
    };

    /**
     * @brief ��ȡԤ�õ�buff��С
     * @return  ����������Ϣbuff��󳤶�
     */
    int GetMsgBuffSize()     {
        return (_buff_size > 0) ? _buff_size : 65535;
    }	 

    /**
     * @brief ���ó�����session������id
     * @return  0�ɹ�
     */
    void SetSessionName(int name) {
        _ntfy_name = name;
    };

    /**
     * @brief ��ȡ����session������id
     * @return  session ע����
     */
    int GetSessionName()     {
        return _ntfy_name;
    }	 

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
     * @brief ���ñ��δ��������������Ϣ
     */
    void SetConnType(MULTI_CONNECT type) {
        _conn_type = type;
    };

    /**
     * @brief ��ȡ���δ��������������Ϣ
     * @return conn type
     */
    MULTI_CONNECT GetConnType() {
        return _conn_type;
    };     

    /**
     * @brief ���ñ��δ����errno
     */
    void SetErrno(MULTI_ERROR err) {
        _errno = err; 
    };

    /**
     * @brief ��ȡ���δ����ERRNO��Ϣ
     * @return ERRONO
     */
    MULTI_ERROR GetErrno() {
        return _errno;
    };     

    /**
     * @brief ���ñ��δ����timecost
     */
    void SetCost(int cost) {
        _time_cost = cost;
    };

    /**
     * @brief ��ȡ���δ����timecost��Ϣ
     * @return timecost
     */
    int GetCost() {
        return _time_cost;
    }; 

	/**
	 * @brief ���ô���״̬��Ϣ
     * @param  flag -��Ϣ����״̬
	 */
	void SetMsgFlag(MULTI_STATE flag) {
        _flag = flag;
	};
	 
    /**
     * @brief ��ȡ����״̬��Ϣ
     * @return flag -��Ϣ����״̬
     */
    MULTI_STATE GetMsgFlag() {
        return _flag;
    };

    /**
     * @brief �����ڲ���Ϣָ��
     * @return IMtConnָ��
     */
    void SetIMsgPtr(IMtMsg* msg  ) {
        _msg = msg;
    };

    /**
     * @brief ��ȡ�ڲ���Ϣָ��
     * @return IMtConnָ��
     */
    IMtMsg* GetIMsgPtr() {
        return _msg;
    };
     
    /**
     * @brief �����ڲ�������ָ��
     * @return IMtConnָ��
     */
    void SetIConnection(IMtConnection* conn) {
        _conn = conn;
    };

    /**
     * @brief ��ȡ�ڲ�������ָ��
     * @return IMtConnָ��
     */
    IMtConnection* GetIConnection() {
        return _conn;
    };

    /**
     * @brief ��ʼ����Ҫ�ֶ���Ϣ
     */
    void Init();

    /**
     * @brief ������, ����Action״̬
     */
    void Reset();

    /**
     * @brief ��ȡ���Ӷ���, ֪ͨ����, ��Ϣ����
     */
    EpollerObj* GetNtfyObj();

    /**
     * @brief ��ȡ���Ӷ���, ֪ͨ����, ��Ϣ����
     */
    int InitConnEnv();

    /**
     * @brief �����麯��, �򻯽ӿ���ʵ�ֲ���
     */
    int DoEncode();
    int DoInput();
    int DoProcess();
    int DoError();

public:

    /**
     * @brief �������ӵ���Ϣ����ӿ�
     * @return >0 -�ɹ�, < 0 ʧ�� 
     */
    virtual int HandleEncode(void* buf, int& len, IMtMsg* msg){return 0;};

    /**
     * @brief �������ӵ�CHECK�ӿ�, TCP�ķְ��ӿ�
     * @return > 0 �Ѿ��ɹ�����,������������С, =0 �����ȴ�, <0 ����(����-65535 UDP����) 
     */
    virtual int HandleInput(void* buf, int len, IMtMsg* msg){return 0;};

    /**
     * @brief �������ӵ�Ӧ����ӿ�, ����һ�������ֶΰ������
     * @return 0 �ɹ�, ����ʧ��
     */
    virtual int HandleProcess(void* buf, int len, IMtMsg* msg){return 0;};

    /**
     * @brief �������Ӵ���Ĵ���֪ͨ, ����μ� MULTI_ERROR ö��
     * @info  ��handleprocessʧ��, �����쳣�����øýӿ�
     * @return 0 �ɹ�, ����ʧ��
     */
    virtual int HandleError(int err, IMtMsg* msg){return 0;};


protected:

    MULTI_STATE         _flag;      // ������������Ϣ, ��ǰ״̬��Ϣ
    MULTI_PROTO         _proto;     // Э������ UDP/TCP
    MULTI_CONNECT       _conn_type; // �������� ��������
	MULTI_ERROR         _errno;     // ��������Ϣ, 0�ɹ���������	
	struct sockaddr_in  _addr;      // ����ʱ��д��ָ�����͵�stAddr	
	int                 _time_cost; // ��������Ӧ���ʱ, ����
	int                 _buff_size; // �����������������Ӧ�𳤶�
	int                 _ntfy_name; // ������session ntfy������, sessionģ������
	
	IMtMsg*             _msg;       // ��Ϣָ��, �ϼ�ָ��
	IMtConnection*      _conn;      // ������ָ��, �¼�ָ��, ����������

};

}

#endif

