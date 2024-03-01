
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
        int iShiftType;    // 0 -> ��С,  1 -> �ļ���, 2 -> shift by interval, 3 ->��, 4 -> Сʱ, 5 ->���� 
        int iMaxLogNum;
        int lMaxSize;
        int lMaxCount;
        int lLogCount;
        time_t lLastShiftTime;
		/*���Ա��滺�����Ϣ*/
		uint32_t dwMaxBufferBytes; /*��󻺴���ֽ���*/
		uint32_t dwMaxBufferTime;  /*���Ļ���ʱ��*/
		uint32_t dwSavedSize; /*�ѻ����*/
		uint64_t ddwLastFlushTime; /*��һ��ˢ����־��ʱ��*/
    } LS_LogFile ;

#define LS_SHM_LS_LOG_BUF_LEN 2000000
#define LS_MAX_SHM_LOG_REC_LEN 65535

    typedef struct
    {
        unsigned int lHeadPos;
        unsigned int lTailPos; //��һ�����õ�λ��
        char sReserved[20];
        char sLogBuf[LS_SHM_LS_LOG_BUF_LEN];
    } LS_LogShm;

    typedef struct
    {
        LS_LogFile stLogFile;
        LS_LogShm *pstLogShm;
        int lMaxLogRec;        //���Dump�������¼��
        char sReserved[20];
    } LS_LogShmFile;

//
#define LS_LOG_TAIL 		(unsigned char)0    //��־��εĽ���
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

/*������־������������*/
#define LOGSYS_LOG_FILE_MAX_BUFF_SIZE 40960
/*Ĭ�ϵ���󻺴�ʱ��*/
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
    //LOG �ȵ� LS_InitLogFile ��ʼ��
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
	//����, "ע��", log������, [���ݵĳ���,]
	LS_LOG_CHAR, "cmd type:", 5,
	LS_LOG_INT, "BufLen type:", 530,
	LS_LOG_STRING, "Msg:", "Hello world\n",
	LS_LOG_NSTRING, "Msg2:", sMsg, strlen(sMsg),
	LS_LOG_BUF, "Pkg:", &stRpkg, sizeof(stRpkg),
	LS_LOG_TAIL);
	 ***/
    int LS_SLogBin(int iLogTime, ...);

    /*
     *lMaxLogNum ����ļ���Ŀ
     *lMAX  ÿ���ļ���󳤶�
     *lMaxLogRec Dump�ڴ浽�ļ�ʱ���dump�ļ�¼��,0��ʾ������
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



// @brief   PkgFlowLog: �����ĸ�ʽ������ݵ�Buffer
//          �洢��ʽ = head[|...]
//          head = cStoreVer|dwServerType|cOpType|dwUin|dwUpdateTime|dwClientIP|dwServiceConnIP|dwServiceSvrIP|wClientType|dwClientVersion
//          ����ʾ����1|3|1|69916789|1272342893|10.130.8.11|10.131.8.1|10.132.9.1|17|86|50|test by vitolin|80
// @param   pFlowLogBuff: bufferָ��
// @param   piFlowLogBuffLen: ���룺buffer����󳤶ȣ�����ȷ��buffer�㹻�����籣��head����buffer�����Ҫ124byte
//                            �����buffer���ݵ�ʵ�ʳ���
// @param   cStoreVer: ��ʽ�汾��
// @param   dwServerType: ҵ������
// @param   cOpType: ��������.
// @param   dwUin: uin
// @param   dwUpdateTime: ���ݱ��ʱ��.
// @param   dwClientIP: �ͻ���IP
// @param   dwServiceConnIP: ҵ���ķ���IP
// @param   dwServiceSvrIP: ҵ���Ľ���IP
// @param   wClientType: �ͻ�������
// @param   dwClientVersion: �ͻ��˰汾
// @param   sFormat: ��ʽ�ַ���
// @param   ...: ���
// @return  �ɹ�����0��ʧ�� < 0
int LS_PkgFlowLog(char *pFlowLogBuff, int *piFlowLogBuffLen,
        char cStoreVer, unsigned int dwServerType, unsigned char cOpType, unsigned int dwUin, unsigned int dwUpdateTime, 
        unsigned int dwClientIP, unsigned int dwServiceConnIP, unsigned int dwServiceSvrIP, 
        unsigned short wClientType, unsigned int dwClientVersion, const char* sFormat, ...);

int LS_SetLogFileBufferTime(LS_LogFile * pstLogFile, uint32_t dwMaxBufferTime);
int LS_SetLogFileBufferByte(LS_LogFile * pstLogFile, uint32_t dwMaxBufferBytes);


#pragma pack(1)
/* ��������API */
typedef struct
{
    unsigned short      wPkgLen;
    char                cPtlVer;    //Э��汾����0
    unsigned short      wBusinessType;  //ҵ������
    unsigned int        dwUin;
    unsigned int        dwFUin;
    unsigned int        dwDelUinCenterIP; //��������IP
    unsigned short      wDelUinCenterRort;
    unsigned int        dwBusinessIP;     //ҵ��IP
    unsigned short      wBusinessPort;
    unsigned int        dwInterfaceIP;        //interface IP
    unsigned short      wInterfacePort;
    unsigned int        dwTimeStamp;
    char sReserved[8];
} LS_TRASH_HEAD;
#pragma pack()

// @brief   SendDeletedInfo: ����ɾ���û����ݰ�����������
// @param   ��������IP��ַΪ������������ֵΪ������
// @param   dwLocalIP: ����IP��ַ
// @param   dwTrashCenterIP: ��������IP��ַ
// @param   dwTrashCenterPort: �������Ķ˿�
// @param   wBusinessType: ҵ������
// @param   dwUin: uin
// @param   dwFUin: friend uin, û�еĻ�����Ϊ0
// @param   dwInterfaceIP: ҵ���Ľӿڻ�IP��ַ
// @param   dwDelUinCenterIP: �������ĵ�IP��ַ
// @param   pBuf: ҵ���������
// @param   wBufLen: ҵ��������ݵĳ���
// @return  �ɹ�����0��ʧ�� < 0
int
LS_SendDeletedInfo(unsigned int dwLocalIP, unsigned int dwTrashCenterIP, unsigned short wTrashCenterPort,
    unsigned short wBusinessType, unsigned int dwUin, unsigned int dwFUin,
    unsigned int dwInterfaceIP, unsigned int dwDelUinCenterIP,
    char *pBuf, unsigned short wBufLen);


// @brief   LS_DeletedInfoCallback: ҵ����������ݵĻص��������������ṩ�ĺ�������IterateDeletedInfo���е���
// @param   pHead: �������ݵ�ͷ��ַ
// @param   pBuf: ҵ��������ݵ�����
// @param   wBufLen: ҵ��������ݵĳ���
// @param   pCtx: �ص������ڲ�����������,��������NULL
// @return  �ɹ�����ҵ��������ݷ���0��ʧ�ܷ��ظ���������ֹIterateDeletedInfo�ı���
typedef int (* LS_DeletedInfoCallback)(const LS_TRASH_HEAD *pHead, const char *pBuf, const unsigned short wBufLen, const void *pCtx);

// @brief   IterateDeletedInfo: ����ҵ����������ļ������Է��ϵ���Ŀ���ûص��������д���
// @param   pszDelFileName: ҵ����������ļ���
// @param   pCallback: �ص�����
// @param   pCtx: �ص������ڲ�����������,��������NULL
// @param   wBusinessType: ��Ҫ���˵�ҵ������
// @param   dwUin: ��Ҫ���˵�uin, 0xffffffff��ʾ���е�uin
// @param   dwFUin: ��Ҫ���˵�friend uin, û�еĻ�����Ϊ0��0xffffffff��ʾ���е�fuin
// @return  �ɹ���������ҵ��������ݷ���0��ʧ�ܷ��ظ���
int 
LS_IterateDeletedInfo(const char *pszDelFileName, LS_DeletedInfoCallback pCallback, void *pCtx,
    unsigned short wBusinessType, unsigned int dwUin, unsigned int dwFUin);


}


#endif
