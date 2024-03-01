#ifndef _COMM_INFO_H
#define _COMM_INFO_H

#define PWD_BUF_LEN 256
#define TOKEN_BUF_LEN 300

//proxy statobj
#define RX_BYTES "rx_bytes"
#define TX_BYTES "tx_bytes"
#define CONN_OVERLOAD "conn_overload"
#define SHM_ERROR "shm_error"
#define CUR_CONN "cur_connects"

//worker statobj
#define SHM_RX_BYTES "shm_rx_bytes"
#define SHM_TX_BYTES "shm_tx_bytes"
#define MSG_TIMEOUT "msg_timeout"
#define MSG_SHM_TIME "msg_shm_time"

//mt statobj
#define MT_THREAD_NUM "mt_thread_num"
#define MT_MSG_NUM "mt_msg_num"

namespace spp
{

enum CoreCheckPoint
{
    CoreCheckPoint_SppFrame         = 0,
    CoreCheckPoint_HandleInit,
    CoreCheckPoint_HandleProcess,
    CoreCheckPoint_HandleFini,
};

namespace statdef
{
//统计项索引，快速访问StatObj的数组下标
typedef enum DefaultProxyStatObjIndex
{
    PIDX_RX_BYTES = 0,
    PIDX_TX_BYTES,              //1
    PIDX_CONN_OVERLOAD,         //2
    PIDX_SHM_ERROR,             //3
    PIDX_CUR_CONN,              //4
} proxy_stat_index;

typedef enum DefaultWorkerStatObjIndex
{
    WIDX_SHM_RX_BYTES = 0,
    WIDX_SHM_TX_BYTES,          //1
    WIDX_MSG_TIMEOUT,
    WIDX_MSG_SHM_TIME,
    
	WIDX_MT_THREAD_NUM,	
	WIDX_MT_MSG_NUM,
} worker_stat_index;
}
}

#endif

