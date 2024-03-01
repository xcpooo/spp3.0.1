/**
 * @brief 统一的协议管理处理
 */

#ifndef _NEST_PROTO_H__
#define _NEST_PROTO_H__

#include <stdint.h>
#include <string>
#include "nest.pb.h"

using std::string;

#define  NEST_AGENT_SET_FILE     "/tmp/.nest_set_file"      // SET 文件路径
#define  NEST_BIN_PATH           "/usr/local/services/"     // bin 安装包路径
#define  NEST_AGENT_UNIX_PATH    "/tmp/.nest_agent"         // agent 本地侦听
#define  NEST_THREAD_UNIX_PATH   "/tmp/.nest_thread"        // thread 本地侦听
#define  NEST_PROC_UNIX_PATH     "/tmp/.nest_process"       // 业务进程本地侦听
#define  NEST_DC_TOOL            "spp_dc_tool"              // dctool文件名
#define  NEST_CTRL_lOG           "/log/spp_frame_ctrl.log"  // dctool扫描文件

#define  NEST_CLONE_UNIX_PATH    "/tmp/.nest_clone"         // clone 进程本地监听 UDP

/**
 * @brief 鸟巢主协议类型
 */
enum nest_proto_main_cmd
{
    NEST_PROTO_TYPE_ADMIN        =   1,   // 配置管理
    NEST_PROTO_TYPE_SCHEDULE     =   2,   // 调度管理
    NEST_PROTO_TYPE_PROC         =   3,   // 进程管理
    NEST_PROTO_TYPE_AGENT        =   4,   // 代理内部
};


/**
 * @brief 鸟巢配置管理子类型
 */
enum nest_proto_admin
{
    NEST_ADMIN_CMD_ADD_NODE      =   1,   // 增加节点
    NEST_ADMIN_CMD_DEL_NODE      =   2,   // 删除节点
    NEST_ADMIN_CMD_ADD_SERVICE   =   3,   // 增加业务
    NEST_ADMIN_CMD_DEL_SERVICE   =   4,   // 下架业务
    NEST_ADMIN_CMD_NODE_INFO     =   5,   // 节点查询
    NEST_ADMIN_CMD_SERVICE_INFO  =   6,   // 业务查询
    NEST_ADMIN_CMD_EXPANSION     =   7,   // 业务扩容
    NEST_ADMIN_CMD_REDUCTION     =   8,   // 业务缩容
};


/**
 * @brief 鸟巢调度管理子类型
 */
enum nest_proto_schedule
{
    NEST_SCHED_CMD_NODE_INIT     =   1,   // 节点初始化
    NEST_SCHED_CMD_NODE_TERM     =   2,   // 节点重置
    NEST_SCHED_CMD_LOAD_REPORT   =   3,   // 节点负载
    NEST_SCHED_CMD_ADD_PROC      =   4,   // 增加业务进程
    NEST_SCHED_CMD_DEL_PROC      =   5,   // 删除业务进程
    NEST_SCHED_CMD_RESTART_PROC  =   6,   // 重启业务进程
    NEST_SCHED_CMD_SERVICE_INFO  =   7,   // 获取业务信息
    NEST_SCHED_CMD_FORK_PROC     =   8,   // fork业务进程
};


/**
 * @brief 鸟巢进程管理子类型
 */
enum nest_proto_process
{
    NEST_PROC_CMD_HEARTBEAT      =   1,   // 进程心跳
    NEST_PROC_CMD_INIT_INFO      =   2,   // 连接关系
    NEST_PROC_CMD_SUSPEND        =   3,   // 进程暂停
    NEST_PROC_CMD_STOP           =   4,   // 进程终止
    NEST_PROC_CMD_STAT_REPORT    =   5,   // 统计上报
};


// 全局的协议返回码定义
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


// 全局的监听端口预设值
#define  NEST_SERVER_PORT         2200
#define  NEST_AGENT_PORT          2014

// 通用包头的起始字符"[" "]"
#define  NEST_PKG_STX      0x5b           
#define  NEST_PKG_ETX      0x5d

// 通用包体的格式 "[" + dwHeadLen + dwBodyLen + sz[head] + sz[body] + "]"
#define  NEST_PKG_COMM_HEAD_LEN   (2 + sizeof(uint32_t)*2)
#define  NEST_PKG_HEAD_LEN_POS    (1UL)
#define  NEST_PKG_BODY_LEN_POS    (5UL)
#define  NEST_PKG_HEAD_BUFF_POS   (9UL)

// 通用msg解析过程定义
typedef struct _NestPkgInfo
{
    void*       msg_buff;           // 消息起始buff指针
    uint32_t    buff_len;           // 消息buff有效长度
    void*       head_buff;          // head buff指针
    uint32_t    head_len;           // head有效长度
    void*       body_buff;          // body buff指针
    uint32_t    body_len;           // body有效长度
}NestPkgInfo;


/**
 * @brief 检查报文的格式合法性, 返回报文长度
 * @param  pkg_info -报文基本信息 modify
 * @param  errmsg -异常信息
 * @return 0 继续等待报文, >0  完成接收, 报文有效长度, <0 失败
 */
int32_t NestProtoCheck(NestPkgInfo* pkg_info, string& err_msg);

/**
 * @brief 封装报文, 返回报文长度
 * @param  pkg_info -报文基本信息
 * @param  errmsg -异常信息
 * @return >0 报文有效长度, <0 失败
 */
int32_t NestProtoPackPkg(NestPkgInfo* pkg_info, string& err_msg);


#endif


