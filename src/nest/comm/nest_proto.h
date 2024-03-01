/**
 * @brief ͳһ��Э�������
 */

#ifndef _NEST_PROTO_H__
#define _NEST_PROTO_H__

#include <stdint.h>
#include <string>
#include "nest.pb.h"

using std::string;

#define  NEST_AGENT_SET_FILE     "/tmp/.nest_set_file"      // SET �ļ�·��
#define  NEST_BIN_PATH           "/usr/local/services/"     // bin ��װ��·��
#define  NEST_AGENT_UNIX_PATH    "/tmp/.nest_agent"         // agent ��������
#define  NEST_THREAD_UNIX_PATH   "/tmp/.nest_thread"        // thread ��������
#define  NEST_PROC_UNIX_PATH     "/tmp/.nest_process"       // ҵ����̱�������
#define  NEST_DC_TOOL            "spp_dc_tool"              // dctool�ļ���
#define  NEST_CTRL_lOG           "/log/spp_frame_ctrl.log"  // dctoolɨ���ļ�

#define  NEST_CLONE_UNIX_PATH    "/tmp/.nest_clone"         // clone ���̱��ؼ��� UDP

/**
 * @brief ����Э������
 */
enum nest_proto_main_cmd
{
    NEST_PROTO_TYPE_ADMIN        =   1,   // ���ù���
    NEST_PROTO_TYPE_SCHEDULE     =   2,   // ���ȹ���
    NEST_PROTO_TYPE_PROC         =   3,   // ���̹���
    NEST_PROTO_TYPE_AGENT        =   4,   // �����ڲ�
};


/**
 * @brief �����ù���������
 */
enum nest_proto_admin
{
    NEST_ADMIN_CMD_ADD_NODE      =   1,   // ���ӽڵ�
    NEST_ADMIN_CMD_DEL_NODE      =   2,   // ɾ���ڵ�
    NEST_ADMIN_CMD_ADD_SERVICE   =   3,   // ����ҵ��
    NEST_ADMIN_CMD_DEL_SERVICE   =   4,   // �¼�ҵ��
    NEST_ADMIN_CMD_NODE_INFO     =   5,   // �ڵ��ѯ
    NEST_ADMIN_CMD_SERVICE_INFO  =   6,   // ҵ���ѯ
    NEST_ADMIN_CMD_EXPANSION     =   7,   // ҵ������
    NEST_ADMIN_CMD_REDUCTION     =   8,   // ҵ������
};


/**
 * @brief �񳲵��ȹ���������
 */
enum nest_proto_schedule
{
    NEST_SCHED_CMD_NODE_INIT     =   1,   // �ڵ��ʼ��
    NEST_SCHED_CMD_NODE_TERM     =   2,   // �ڵ�����
    NEST_SCHED_CMD_LOAD_REPORT   =   3,   // �ڵ㸺��
    NEST_SCHED_CMD_ADD_PROC      =   4,   // ����ҵ�����
    NEST_SCHED_CMD_DEL_PROC      =   5,   // ɾ��ҵ�����
    NEST_SCHED_CMD_RESTART_PROC  =   6,   // ����ҵ�����
    NEST_SCHED_CMD_SERVICE_INFO  =   7,   // ��ȡҵ����Ϣ
    NEST_SCHED_CMD_FORK_PROC     =   8,   // forkҵ�����
};


/**
 * @brief �񳲽��̹���������
 */
enum nest_proto_process
{
    NEST_PROC_CMD_HEARTBEAT      =   1,   // ��������
    NEST_PROC_CMD_INIT_INFO      =   2,   // ���ӹ�ϵ
    NEST_PROC_CMD_SUSPEND        =   3,   // ������ͣ
    NEST_PROC_CMD_STOP           =   4,   // ������ֹ
    NEST_PROC_CMD_STAT_REPORT    =   5,   // ͳ���ϱ�
};


// ȫ�ֵ�Э�鷵���붨��
enum proto_errno
{
    NEST_RESULT_SUCCESS              =  0,
    NEST_ERROR_INVALID_BODY          =  1,
    NEST_ERROR_MEM_ALLOC_ERROR       =  2,
    NEST_ERROR_START_PROC_ERROR      =  3,
    NEST_ERROR_PROC_ALREADY_USED     =  4,
    NEST_ERROR_PROC_THREAD_FAILED    =  5,

    
    NEST_ERROR_NODE_SET_NO_MATCH     =  1001,
    NEST_ERROR_NODE_INIT_FAILED      =  1002,
    NEST_ERROR_NODE_DEL_HAS_PROC     =  1003,
};


// ȫ�ֵļ����˿�Ԥ��ֵ
#define  NEST_SERVER_PORT         2200
#define  NEST_AGENT_PORT          2014

// ͨ�ð�ͷ����ʼ�ַ�"[" "]"
#define  NEST_PKG_STX      0x5b           
#define  NEST_PKG_ETX      0x5d

// ͨ�ð���ĸ�ʽ "[" + dwHeadLen + dwBodyLen + sz[head] + sz[body] + "]"
#define  NEST_PKG_COMM_HEAD_LEN   (2 + sizeof(uint32_t)*2)
#define  NEST_PKG_HEAD_LEN_POS    (1UL)
#define  NEST_PKG_BODY_LEN_POS    (5UL)
#define  NEST_PKG_HEAD_BUFF_POS   (9UL)

// ͨ��msg�������̶���
typedef struct _NestPkgInfo
{
    void*       msg_buff;           // ��Ϣ��ʼbuffָ��
    uint32_t    buff_len;           // ��Ϣbuff��Ч����
    void*       head_buff;          // head buffָ��
    uint32_t    head_len;           // head��Ч����
    void*       body_buff;          // body buffָ��
    uint32_t    body_len;           // body��Ч����
}NestPkgInfo;


/**
 * @brief ��鱨�ĵĸ�ʽ�Ϸ���, ���ر��ĳ���
 * @param  pkg_info -���Ļ�����Ϣ modify
 * @param  errmsg -�쳣��Ϣ
 * @return 0 �����ȴ�����, >0  ��ɽ���, ������Ч����, <0 ʧ��
 */
int32_t NestProtoCheck(NestPkgInfo* pkg_info, string& err_msg);

/**
 * @brief ��װ����, ���ر��ĳ���
 * @param  pkg_info -���Ļ�����Ϣ
 * @param  errmsg -�쳣��Ϣ
 * @return >0 ������Ч����, <0 ʧ��
 */
int32_t NestProtoPackPkg(NestPkgInfo* pkg_info, string& err_msg);


#endif


