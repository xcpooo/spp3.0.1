/**
 *  @file mt_concurrent.h
 *  @info ��չ״̬�̵߳Ĵ���ģ��
 *  @time 20130515
 **/

#ifndef __MT_CONCURRENT_H__
#define __MT_CONCURRENT_H__

#include <netinet/in.h>
#include <vector>

namespace NS_MICRO_THREAD {

using std::vector;

class IMtAction;
typedef vector<IMtAction*>  IMtActList;

/******************************************************************************/
/*  ΢�߳��û��ӿڶ���: ΢�߳�Action��·����ģ�ͽӿڶ���                      */
/******************************************************************************/

/**
 * @brief ��·IO�������մ���ӿ�, ��װACTON�ӿ�ģ��, �ڲ�����msg
 * @param req_list -action list ʵ�ַ�װ�����ӿ�
 * @param timeout -��ʱʱ��, ��λms
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -100 ����Ӧ�𲿷�ʧ��, �ɴ�ӡerrno
 */
int mt_msg_sendrcv(IMtActList& req_list, int timeout);

/******************************************************************************/
/*  �ڲ�ʵ�ֶ��岿��                                                          */
/******************************************************************************/

/**
 * @brief ��·IO�Ĵ����Ż�, �첽���ȵȴ�����
 * @param req_list - �����б�
 * @param how - EPOLLIN  EPOLLOUT
 * @param timeout - ��ʱʱ�� ���뵥λ
 * @return 0 �ɹ�, <0ʧ�� -3 ����ʱ
 */
int mt_multi_netfd_poll(IMtActList& req_list, int how, int timeout);

/**
 * @brief Ϊÿ��ITEM���������ĵ�socket
 * @param req_list - �����б�
 * @return 0 �ɹ�, <0ʧ��
 */
int mt_multi_newsock(IMtActList& req_list);

/**
 * @brief ��·IO�Ĵ���, ������
 * @param req_list - �����б�
 * @param timeout - ��ʱʱ�� ���뵥λ
 * @return 0 �ɹ�, <0ʧ��
 */
int mt_multi_open(IMtActList& req_list, int timeout);

/**
 * @brief ��·IO�Ĵ���, ��������
 * @param req_list - �����б�
 * @param timeout - ��ʱʱ�� ���뵥λ
 * @return 0 �ɹ�, <0ʧ��
 */
int mt_multi_sendto(IMtActList& req_list, int timeout);

/**
 * @brief ��·IO�������մ���
 */
int mt_multi_recvfrom(IMtActList& req_list, int timeout);

/**
 * @brief ��·IO�������մ���
 */
int mt_multi_sendrcv_ex(IMtActList& req_list, int timeout);

}



#endif


