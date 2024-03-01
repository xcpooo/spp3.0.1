/**
 * @file nest_proto.cpp
 * @info protobuff comm
 */

#include <arpa/inet.h> 
#include <stdio.h>
#include "nest_proto.h"

/**
 * @brief ��鱨�ĵĸ�ʽ�Ϸ���, ���ر��ĳ���
 * @param  buff -������ʼ��ַ
 * @param  len  -���ĳ���
 * @param  errmsg -�쳣��Ϣ
 * @return 0 �����ȴ�����, >0  ��ɽ���, ������Ч����, <0 ʧ��
 */
int32_t NestProtoCheck(NestPkgInfo* pkg_info, string& err_msg)
{
    if (!pkg_info || !pkg_info->msg_buff) 
    {
        err_msg = "input param pkg info invalid";
        return -1;
    }
    
    char  szMsg[256];
    char* pCur = (char*)pkg_info->msg_buff;
    uint32_t len = pkg_info->buff_len;
    
    // 1. ��鱨�ĳ�������β
    if (len < NEST_PKG_COMM_HEAD_LEN)
    {
        return 0;
    }

    // 2. ����ͷ��Ϣ���
    if (pCur[0] != NEST_PKG_STX)
    {
        snprintf(szMsg, sizeof(szMsg), "check package failed, stx: %d invalid", pCur[0]);
        err_msg = szMsg;
        return -2;
    }

    // 3. ��ȡ������Ϣ
    uint32_t dwHeadLen = ntohl(*(uint32_t *)(pCur + NEST_PKG_HEAD_LEN_POS));
    uint32_t dwBodyLen = ntohl(*(uint32_t *)(pCur + NEST_PKG_BODY_LEN_POS));
    uint32_t dwMsgLen  = NEST_PKG_COMM_HEAD_LEN + dwHeadLen + dwBodyLen;
    if (dwMsgLen > len)
    {
        return 0;
    }

    // 4. ��β��Ϣ���
    if (pCur[dwMsgLen - 1] != NEST_PKG_ETX)
    {
        snprintf(szMsg, sizeof(szMsg), "check package failed, etx: %d invalid", pCur[dwMsgLen - 1]);
        err_msg = szMsg;
        return -3;
    }

    // 5. ���Ļ�����Ϣ�Ϸ�, ���س���
    pkg_info->head_buff = (char*)pkg_info->msg_buff + NEST_PKG_HEAD_BUFF_POS;
    pkg_info->head_len  = dwHeadLen;
    pkg_info->body_buff = (char*)pkg_info->head_buff + dwHeadLen;
    pkg_info->body_len  = dwBodyLen;    
    
    return (int32_t)dwMsgLen;    
}


/**
 * @brief ��װ����, ���ر��ĳ���
 * @param  pkg_info -���Ļ�����Ϣ
 * @param  errmsg -�쳣��Ϣ
 * @return >0 ������Ч����, <0 ʧ��
 */
int32_t NestProtoPackPkg(NestPkgInfo* pkg_info, string& err_msg)
{
    if (!pkg_info || !pkg_info->head_buff) 
    {
        err_msg = "input param pkg info invalid";
        return -1;
    }

    char* pcur = (char*)pkg_info->msg_buff;
    uint32_t left =  pkg_info->buff_len;
    uint32_t msg_len = NEST_PKG_COMM_HEAD_LEN + pkg_info->head_len + pkg_info->body_len;
    if (left < msg_len)
    {
        err_msg = "input buff len too small";
        return -2;
    }

    *pcur = NEST_PKG_STX;
    *(uint32_t *)(pcur + NEST_PKG_HEAD_LEN_POS) = htonl(pkg_info->head_len);
    *(uint32_t *)(pcur + NEST_PKG_BODY_LEN_POS) = htonl(pkg_info->body_len);
    memcpy(pcur + NEST_PKG_HEAD_BUFF_POS, pkg_info->head_buff, pkg_info->head_len);
    if ((pkg_info->body_len > 0) && (pkg_info->body_buff != NULL))
    {
        memcpy(pcur + NEST_PKG_HEAD_BUFF_POS + pkg_info->head_len,
               pkg_info->body_buff, pkg_info->body_len);
    }
    *(pcur + msg_len -1) = NEST_PKG_ETX;

    return (int32_t)msg_len;
}





