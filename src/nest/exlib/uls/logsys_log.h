
#ifndef _LS_LOG_H
#define _LS_LOG_H

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE   //for LFS support
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64    //for LFS support
#endif

#define LOGSYS_LOG_MAX_BASE_FILE_NAME_LEN 256

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define LOCK_API_SHIFT_FILE_KEY 0x10910920
namespace ULS
{
    typedef struct
    {
        FILE *pLogFile;
        char sBaseFileName[LOGSYS_LOG_MAX_BASE_FILE_NAME_LEN];
        char sLogFileName[LOGSYS_LOG_MAX_BASE_FILE_NAME_LEN];
        int iShiftType;    // 0 -> 大小,  1 -> 文件数, 2 -> shift by interval, 3 ->天, 4 -> 小时, 5 ->分钟 
        int iMaxLogNum;
        int lMaxSize;
        int lMaxCount;
        int lLogCount;
        time_t lLastShiftTime;
		/*用以保存缓存的信息*/
		uint32_t dwMaxBufferBytes; /*最大缓存的字节数*/
		uint32_t dwMaxBufferTime;  /*最大的缓存时间*/
		uint32_t dwSavedSize; /*已缓存的*/
		uint64_t ddwLastFlushTime; /*上一次刷新日志的时间*/
    } LS_LogFile ;

#define LS_SHM_LS_LOG_BUF_LEN 2000000
#define LS_MAX_SHM_LOG_REC_LEN 65535

    typedef struct
    {
        unsigned int lHeadPos;
        unsigned int lTailPos; //下一个可用的位置
        char sReserved[20];
        char sLogBuf[LS_SHM_LS_LOG_BUF_LEN];
    } LS_LogShm;

    typedef struct
    {
        LS_LogFile stLogFile;
        LS_LogShm *pstLogShm;
        int lMaxLogRec;        //最多Dump的最近记录数
        char sReserved[20];
    } LS_LogShmFile;

//
#define LS_LOG_TAIL 		(unsigned char)0    //标志变参的结束
#define LS_LOG_CHAR    	(unsigned char)1
#define LS_LOG_SHORT    	(unsigned char)2
#define LS_LOG_INT     	(unsigned char)3
#define LS_LOG_LONG    	(unsigned char)4
#define LS_LOG_DOUBLE  	(unsigned char)5
#define LS_LOG_STRING  	(unsigned char)6
#define LS_LOG_DESC_STRING (unsigned char)7
#define LS_LOG_NSTRING  	(unsigned char)8
#define LS_LOG_BUF			(unsigned char)9
#define LS_LOG_BUF2		(unsigned char)10
//
#define LS_LOG_TIME0	(unsigned char)65
#define LS_LOG_TIME1	(unsigned char)66
#define LS_LOG_TIME2	(unsigned char)67

#define LS_BASIC_ARGS 	LS_LOG_STRING, "Function: ", __FUNCTION__,LS_LOG_INT,"Line: ", __LINE__ 
#define LS_SLOG0(fmt, args...) LS_SLogBin(0, LS_BASIC_ARGS fmt, ## args, LS_LOG_TAIL)
#define LS_SLOG1(fmt, args...) LS_SLogBin(1, LS_BASIC_ARGS fmt, ## args, LS_LOG_TAIL)
#define LS_SLOG2(fmt, args...) LS_SLogBin(2, LS_BASIC_ARGS fmt, ## args, LS_LOG_TAIL)
#define LS_SLOG LS_SLOG1

/*本地日志缓存的相关配置*/
#define LOGSYS_LOG_FILE_MAX_BUFF_SIZE 40960
/*默认的最大缓存时间*/
#define LOGSYS_LOG_FILE_MAX_BUFF_TIME 2
#define LOGSYS_LOG_FILE_INVALID_PARAM 0


#define  LS_LOGINFO(pstLog,fmt, args...)  LS_Log(pstLog, 2, "%s:%d(%s): " fmt, __FILE__, __LINE__, __FUNCTION__ , ## args)

    enum {
        LS_LOGBIN_TIME_SIMPLE = 1,
        LS_LOGBIN_TIME,
        LS_LOGBIN_BUFFER,
    };

    //iShiftType;
    //0->shift by size, 
    //1 or default->shift by LogCount, 
    //2->shift by interval, seconds > iMAX 
    //3->shift by day, 
    //4->shift by hour 
    //5->shift by minute
    //6->shift by day, log filename append with date.
    //LOG 先调 LS_InitLogFile 初始化
    int LS_InitLogFile(LS_LogFile * pstLogFile, char *sLogBaseName, int iShiftType, int iMaxLogNum, int iMAX);
    int LS_Log(LS_LogFile * pstLogFile, int iLogTime, char *sFormat, ...) __attribute__ ((format (printf, 3, 4))) ;
    // @brief   LS_LogBin: Log bin buffer to logfile.
    //          LogBin file struct = [TLV], 
    //              T(4Bytes) = LS_LOGBIN_TIME_SIMPLE/LS_LOGBIN_TIME/LS_LOGBIN_BUFFER. 
    //              L(4Bytes) = length of context.
    //              VALUE(L Bytes)
    // @param pstLogFile    pointer to log file struct.
    // @param iLogTime      0 = do not log time, 1 = log simple time, 2 = log detail time.
    // @param sBuf          buffer to log.
    // @param iBufLen       buffer length.
    // @return <0 means failure.
    int LS_LogBin(LS_LogFile *pstLogFile, int iLogTime, const char *sBuf, int iBufLen);
	/***
	  SlogBin(1,
	//类型, "注释", log的内容, [内容的长度,]
	LS_LOG_CHAR, "cmd type:", 5,
	LS_LOG_INT, "BufLen type:", 530,
	LS_LOG_STRING, "Msg:", "Hello world\n",
	LS_LOG_NSTRING, "Msg2:", sMsg, strlen(sMsg),
	LS_LOG_BUF, "Pkg:", &stRpkg, sizeof(stRpkg),
	LS_LOG_TAIL);
	 ***/
    int LS_SLogBin(int iLogTime, ...);

    /*
     *lMaxLogNum 最多文件数目
     *lMAX  每个文件最大长度
     *lMaxLogRec Dump内存到文件时最多dump的记录数,0表示不限制
     */
    int LS_InitBinLogShmFile(int iShmKey, char *sLogBaseName, int lShiftType, int lMaxLogNum, int lMAX, int lMaxLogRec);

    #define LS_LOG_BUFFER_MAX_LEN 65536 

    // @brief   LogDump: Dump Bin LogFile to screen.
    // @param   pszLogFileName Log Filename
    // @return <0 means failure.
    int LS_LogDump(const char *pszLogFileName);

    // @brief   LogSend: Send Bin LogFile's Buffers to network.
    // @param   pszLogFileName Log Filename
    // @param   sDestIP  destination IP address
    // @param   sDestPort destination Port.
    // @return <0 means failure.
    int LS_LogSend(const char *pszLogFileName, const char *sDestIP, const char *sDestPort);

    // @brief   LS_LogIteratorCallback: user process buffer in this function.
    // @param   time: buffer logging time
    // @param   iLen: buffer len
    // @param   sBuf: buffer address.
    // @return  if process successfully, should return 0. 
    //          otherwise, return negtive value, which will abort iterate process.
    typedef int (* LS_LogIteratorCallback)(struct timeval *time, int iLen, char *sBuf);

    // @brief   LogIterate: Iterate all log buffer.
    // @param   pszLogFileName Log Filename
    // @param   pUserCallback callback function, caller process buffer in this function.
    // @return <0 means failure.
    int LS_LogIterate(const char *pszLogFileName, LS_LogIteratorCallback pUserCallback);



// @brief   PkgFlowLog: 用明文格式打包数据到Buffer
//          存储格式 = head[|...]
//          head = cStoreVer|dwServerType|cOpType|dwUin|dwUpdateTime|dwClientIP|dwServiceConnIP|dwServiceSvrIP|wClientType|dwClientVersion
//          数据示例：1|3|1|69916789|1272342893|10.130.8.11|10.131.8.1|10.132.9.1|17|86|50|test by vitolin|80
// @param   pFlowLogBuff: buffer指针
// @param   piFlowLogBuffLen: 输入：buffer的最大长度，必须确保buffer足够长，如保存head，则buffer最大需要124byte
//                            输出：buffer内容的实际长度
// @param   cStoreVer: 格式版本号
// @param   dwServerType: 业务类型
// @param   cOpType: 操作类型.
// @param   dwUin: uin
// @param   dwUpdateTime: 数据变更时间.
// @param   dwClientIP: 客户端IP
// @param   dwServiceConnIP: 业务侧的发起IP
// @param   dwServiceSvrIP: 业务侧的结束IP
// @param   wClientType: 客户端类型
// @param   dwClientVersion: 客户端版本
// @param   sFormat: 格式字符串
// @param   ...: 变参
// @return  成功返回0，失败 < 0
int LS_PkgFlowLog(char *pFlowLogBuff, int *piFlowLogBuffLen,
        char cStoreVer, unsigned int dwServerType, unsigned char cOpType, unsigned int dwUin, unsigned int dwUpdateTime, 
        unsigned int dwClientIP, unsigned int dwServiceConnIP, unsigned int dwServiceSvrIP, 
        unsigned short wClientType, unsigned int dwClientVersion, const char* sFormat, ...);

int LS_SetLogFileBufferTime(LS_LogFile * pstLogFile, uint32_t dwMaxBufferTime);
int LS_SetLogFileBufferByte(LS_LogFile * pstLogFile, uint32_t dwMaxBufferBytes);


#pragma pack(1)
/* 垃圾中心API */
typedef struct
{
    unsigned short      wPkgLen;
    char                cPtlVer;    //协议版本，填0
    unsigned short      wBusinessType;  //业务类型
    unsigned int        dwUin;
    unsigned int        dwFUin;
    unsigned int        dwDelUinCenterIP; //回收中心IP
    unsigned short      wDelUinCenterRort;
    unsigned int        dwBusinessIP;     //业务IP
    unsigned short      wBusinessPort;
    unsigned int        dwInterfaceIP;        //interface IP
    unsigned short      wInterfacePort;
    unsigned int        dwTimeStamp;
    char sReserved[8];
} LS_TRASH_HEAD;
#pragma pack()

// @brief   SendDeletedInfo: 发送删除用户数据包到垃圾中心
// @param   参数表中IP地址为网络序，其他数值为主机序
// @param   dwLocalIP: 本地IP地址
// @param   dwTrashCenterIP: 垃圾中心IP地址
// @param   dwTrashCenterPort: 垃圾中心端口
// @param   wBusinessType: 业务类型
// @param   dwUin: uin
// @param   dwFUin: friend uin, 没有的话设置为0
// @param   dwInterfaceIP: 业务侧的接口机IP地址
// @param   dwDelUinCenterIP: 回收中心的IP地址
// @param   pBuf: 业务回收数据
// @param   wBufLen: 业务回收数据的长度
// @return  成功返回0，失败 < 0
int
LS_SendDeletedInfo(unsigned int dwLocalIP, unsigned int dwTrashCenterIP, unsigned short wTrashCenterPort,
    unsigned short wBusinessType, unsigned int dwUin, unsigned int dwFUin,
    unsigned int dwInterfaceIP, unsigned int dwDelUinCenterIP,
    char *pBuf, unsigned short wBufLen);


// @brief   LS_DeletedInfoCallback: 业务处理回收数据的回调函数，调用者提供改函数，由IterateDeletedInfo进行调用
// @param   pHead: 回收数据的头地址
// @param   pBuf: 业务回收数据的内容
// @param   wBufLen: 业务回收数据的长度
// @param   pCtx: 回调函数内部处理上下文,不用则填NULL
// @return  成功处理业务回收数据返回0，失败返回负数，将终止IterateDeletedInfo的遍历
typedef int (* LS_DeletedInfoCallback)(const LS_TRASH_HEAD *pHead, const char *pBuf, const unsigned short wBufLen, const void *pCtx);

// @brief   IterateDeletedInfo: 遍历业务回收数据文件，并对符合的项目调用回调函数进行处理
// @param   pszDelFileName: 业务回收数据文件名
// @param   pCallback: 回调函数
// @param   pCtx: 回调函数内部处理上下文,不用则填NULL
// @param   wBusinessType: 需要过滤的业务类型
// @param   dwUin: 需要过滤的uin, 0xffffffff表示所有的uin
// @param   dwFUin: 需要过滤的friend uin, 没有的话设置为0，0xffffffff表示所有的fuin
// @return  成功处理所有业务回收数据返回0，失败返回负数
int 
LS_IterateDeletedInfo(const char *pszDelFileName, LS_DeletedInfoCallback pCallback, void *pCtx,
    unsigned short wBusinessType, unsigned int dwUin, unsigned int dwFUin);


}


#endif
