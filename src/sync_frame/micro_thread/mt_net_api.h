/**
 *  @file mt_net_api.h
 *  @info ΢�̷߳�װ������ӿ���
 **/

#ifndef __MT_NET_API_H__
#define __MT_NET_API_H__

#include <netinet/in.h>

namespace NS_MICRO_THREAD {

/**
 * @brief Э�����Ͷ���
 */
enum MT_PROTO_TYPE 
{
    NET_PROTO_UNDEF      = 0,
    NET_PROTO_UDP        = 0x1,                ///< �������� UDP
    NET_PROTO_TCP        = 0x2                 ///< �������� TCP
};

/**
 * @brief �������Ͷ���
 */
enum MT_RC_TYPE 
{
    RC_SUCCESS          = 0,
    RC_ERR_SOCKET       = -1,           ///< ����socketʧ��
    RC_SEND_FAIL        = -2,           ///< ����ʧ��
    RC_RECV_FAIL        = -3,           ///< ����ʧ��
    RC_CONNECT_FAIL     = -4,           ///< ����ʧ��
    RC_CHECK_PKG_FAIL   = -5,           ///< ���ļ��ʧ��
    RC_NO_MORE_BUFF     = -6,           ///< �ռ䳬������
    RC_REMOTE_CLOSED    = -7,           ///< ��˹ر�����

    RC_INVALID_PARAM    = -10,          ///< ��Ч����
    RC_INVALID_HANDLER  = -11,          ///< ��Ч�ľ��
    RC_MEM_ERROR        = -12,          ///< �ڴ��쳣
    RC_CONFLICT_SID     = -13,          ///< SESSION ID��ͻ
    RC_EPOLL_ERROR      = -14,          ///< rst�źŵ�
};


/**
 * @brief ��鱨���Ƿ�����, ����ȡsession�Ļص�����
 * @info  �ṩneed_len������ԭ��, �����޷�ȷ�ϱ��ĳ���ʱ, ����ÿ����չϣ������
 *        �����������ֵ����������, ���޷������������
 * @param data  -ʵ�ʽ��յ�����ָ��
 * @param len   -�Ѿ����ջ�׼���ĳ���
 * @param session_id -�ɹ�������sessionid��Ϣ
 * @param need_len   -ϣ����չһ��buff, Ŀǰ���100M
 * @return >0 �ɹ���������ʵ�ʵİ�����; =0 ���Ĳ�����, �������ո�������; <0 ����ʧ��
 */
typedef int32_t (*CHECK_SESSION_CALLBACK)(const void* data, uint32_t len,
                                        uint64_t* session_id, uint32_t* need_len);


/**
 * @brief ����ӿ��ඨ��
 */
class CNetHelper
{
public:

    // ת����������Ϣ, �����ȡ
    static char* GetErrMsg(int32_t result);

    // ͬ���շ��ӿ�
    int32_t SendRecv(void* data, uint32_t len, uint32_t timeout);

    // ��ȡ����buff��Ϣ, ��Ч��ֱ��helper����
    void* GetRspBuff();

    // ��ȡ���ذ��ĳ���
    uint32_t GetRspLen();    

    // ����Э�������, Ĭ��UDP
    void SetProtoType(MT_PROTO_TYPE type);

    // ����Ŀ��IP��ַ
	void SetDestAddress(struct sockaddr_in* dst);

	// ����session����session id��Ϣ, �����0
	void SetSessionId(uint64_t sid);	

    // ����session�����ص�����
    void SetSessionCallback(CHECK_SESSION_CALLBACK function);

    // �������鹹
    CNetHelper();
    ~CNetHelper();

private:

    void*    handler;               // ˽�о��, ������չ
};


}

#endif


