/*********************************************************************
 * ��־�࣬�����¼��־
 * **********************************************************************/
#ifndef _LOGSYS_CLIENT_H_
#define _LOGSYS_CLIENT_H_

#include <stdint.h>
#include <stdarg.h>

#include "logsys_log.h"

using namespace ULS;

namespace ULS{


#define LC_VERSION 			1
#define LC_VERSION_2 		2
#define LC_VERSION_3 		3
#define LC_VERSION_4        4

#define LOGSYS_SVR_WRITE_LOG_CMD 0x1
#define LOGSYS_SVR_BATCH_WRITE_LOG_CMD 0x10
#define LOGSYS_MAX_STR_LEN 8192

/* ��־������*/
#define _LC_ERROR_ 		1	//���ش���
#define _LC_WARNING_ 	2 	//����,�Ƚ����أ���Ҫ�����ֻ�
#define _LC_NOTICE_ 		3 	//��Ҫע������⣬�����ǷǷ��İ�֮�࣬����Ҫ���͵��ֻ�����Ϊ���ܺܶ�
#define _LC_INFO_ 			4	//��ˮ
#define _LC_DEBUG_ 		5	//debug


#define LOGSYS_MAX_LOGD_IF_SVR			300		//logsys_if��������������
#define LOGSYS_LOGD_IF_IP_LEN			30           //logsys_if��������IP��󳤶�

#define LOGSYS_SENDTO_WATER			1		//������ˮ��־
#define LOGSYS_SENDTO_COLOR			2	        //����Ⱦɫ��־

#define LS_INVALID_DEST_ID 0 //�Ƿ�����־�ӿڻ�role
#define LS_WATER_DEST_ID				1		//ͨ����ˮ��־�ӿڻ������������е�role
#define LS_COLOR_DEST_ID				2		//ͨ��Ⱦɫ��־�ӿڻ������������е�role

#define	_LC_ERR_RET_ID_		1  //�޸�LsPkgParam��dwErrId
#define	_LC_USER_DEF1_		2  //�޸�LsPkgParam��dwUserDef1
#define	_LC_USER_DEF2_		3  //�޸�LsPkgParam��dwUserDef2
#define	_LC_USER_DEF3_		4  //�޸�LsPkgParam��dwUserDef3
#define	_LC_USER_DEF4_		5  //�޸�LsPkgParam��dwUserDef4

#define LOGSYS_MAX_CMD_STR_LEN 64

#define LOGSYS_STATICS(fmt, args...) do { \
    LsStaticsLog(fmt, ## args); }  while (0)

#define SET_LOGSYS_ERRMSG(szErrMsg) do { \
    LsSetRetErrMsg(szErrMsg); }  while (0)

#define CLEAR_LOGSYS_ERRMSG() do { \
    LsClearRetErrMsg(); }  while (0)

/*
   Ҫʹ������Э���ӡ��صĺ꣬�������ڰ�����ͷ�ļ�֮ǰ����Э���Ӧ��displayͷ�ļ�����"display_XXX.h"
   */
#ifdef _CPPHEAD_PROTO_DEFINE_H
#define LOGSYS_STRUCT(iLogLevel, _struct, prompt) do{\
    if (iLogLevel <= g_stLsClient.m_cWaterLogLocalLevel){\
        if((g_stLsClient.m_stWaterLog.pLogFile = fopen(g_stLsClient.m_stWaterLog.sLogFileName, "a+")) == NULL) break;\
        proto_display(g_stLsClient.m_stWaterLog.pLogFile, _struct, prompt);\
        fclose(g_stLsClient.m_stWaterLog.pLogFile);\
    }\
} while(0)
#define LOGSYS_UNION(iLogLevel, _union, prompt, id, len) do{\
    if (iLogLevel <= g_stLsClient.m_cWaterLogLocalLevel){\
        if((g_stLsClient.m_stWaterLog.pLogFile = fopen(g_stLsClient.m_stWaterLog.sLogFileName, "a+")) == NULL) break;\
        proto_display_union(g_stLsClient.m_stWaterLog.pLogFile, _union, (prompt), (id), (len));\
        fclose(g_stLsClient.m_stWaterLog.pLogFile);\
    }\
} while(0)
#else
#define LOGSYS_STRUCT(iLogLevel, _struct, prompt)
#define LOGSYS_UNION(iLogLevel, _union, prompt, id, len)
#endif

#define LOGSYS_APPID  109
#define LOGSYS_CHANNEL_API_KEY 0x10910931
#define LOGSYS_CHANNEL_API_SIZE 100 * 1024 * 1024

#define LOGSYS_CLIENT_ANTI_SNOW_SHM_KEY     0x41300
#define LOGSYS_CLIENT_COMMON_SHM_KEY     0x40400
#define LOGSYS_CLIENT_COMMON_EX_SHM_KEY	 0x40401

#define LOCAL_LOG_ID 10000
#pragma pack(1)
typedef struct 
{
    uint16_t  wLength;
    uint16_t  wVersion; 
    uint16_t  wCommand;
    uint32_t  dwCmdSequence;
    char  acReserved[4];
}Ls_PkgHead;  

typedef struct
{
	int32_t iLogDst;
	uint16_t wBuffLen;
	char sSendBuff[1024 * 10];
}Ls_ChannelPkg;
#pragma pack()


typedef struct  
{
    uint32_t dwAppId; 			//ҵ���AppId,��ϵͳ����Ա����,�����ֶ�
    uint32_t dwUserDef1;       //��ʼ���û��Զ����ֶ�1���ɵ������Լ���д���Լ�����
    uint32_t dwProcId;
	uint16_t wMsgPort;			//��ָ������Զ����־���󶨵Ķ˿ڣ�Ϊ0������˿�
	uint16_t wRelayPort;		//��ָ��ת�������󶨵Ķ˿ڣ�Ϊ0������˿�
	uint16_t cAgentLocalLog;    //�Ƿ���Agent�������¼����log��ҵ����̲��ټ�¼
}LsInitParam;   //���̳�ʼ��ʱ��ȷ������Ϣ

typedef struct  
{
    uint64_t ddwUin;   //64λuin�������ֶ�,ͬconn��qwUin
    uint32_t dwClientIp; //����ǰ�˷�����û�IP��һ�����û����û�����IP,������ͬconn��dwClientIP
    uint8_t  sClientIpV6[16];  //����ǰ�˷�����û�IP��һ�����û����û�����IP,ipv6 ��ʽ
    uint32_t dwServiceIp;   //ǰ��Ӧ������IP,conn ip��Ϊ��ǰ��Ӧ������IP,������ ��ͬconn��dwServiceIp
    uint32_t dwClientVersion;  //�ͻ��˰汾��,ͬconn��wVersion
    uint32_t dwCmd;      //����ţ�ͬconn��wCmd
    uint32_t dwSubCmd;   //������ţ�ͬconn��dwSubCmd
    int32_t dwErrId;   //��������еĴ��󷵻��룬ͬconn��wErrNo��ÿ�ε��ü�¼��־�� apiʱ��Ҫ����ò���  
    uint32_t dwUserDef1;  //�û��Զ����ֶ�1���ɵ������Լ���д���Լ����ͣ�conn��cPkgType��dwLoginTime��dwLogOutTime��ʹ���Զ����ֶμ�¼������
    uint32_t dwUserDef2;  //�û��Զ����ֶ�2���ɵ������Լ���д���Լ�����
    uint32_t dwUserDef3;  //�û��Զ����ֶ�3���ɵ������Լ���д���Լ�����
    uint32_t dwUserDef4;  //�û��Զ����ֶ�4���ɵ������Լ���д���Լ�����  
    //char szCmdStr[LOGSYS_MAX_CMD_STR_LEN];
}LsPkgParam;   //һ�����������ʼʱ����ȷ������Ϣ

typedef struct
{
	char * szLogFilePath;  /*������־�ļ��������Ȳ��ܳ���55*/
	int iLogSize;
	int iLogNum;
	uint32_t dwMaxBufferBytes; /*����ܻ���ı����ֽ���*/
	uint32_t dwMaxBufferTime;  /*����ܻ����ʱ��*/
}LsLogFileInfo;

typedef struct
{
    /* ������Ϣ, �������һ�ε��õĴ�����Ϣ������������ϸ�ķ���*/	
    char m_szErrMsg[512];
    LS_LogFile  m_stWaterLog;

    /* ��ˮ��־��������������Ϣ*/
    int m_iSvrConfiged;     //0:������Զ�̼�¼��ˮ��־, ��0:����Զ�̼�¼��ˮ��־

    /* Ⱦɫ��־��������������Ϣ*/
    int m_iColorSvrConfiged;   //0:������Զ�̼�¼Ⱦɫ��־, ��0:����Զ�̼�¼Ⱦɫ��־

    /* ��־������Ϣ */ 
    LsInitParam m_stLsInitParam;
    uint64_t m_ddwRandUin;

    /* ������ˮ��־����־��¼����С�ڵ��ڸü���Ķ���¼������*/
    int8_t m_cWaterLogLocalLevel;
    /* ��ˮ��־���ĵ���־��¼����С�ڵ��ڸü���Ķ����͵���ˮ��־����, һ������Ϊ_LOGSYS_WARNING_*/
    int8_t m_cWaterLogMsgLevel;

    /*ͳ����־*/
    LS_LogFile  m_stStaticsLog;

    /* debug ��־*/
    LS_LogFile  m_stTraceLog;
    /* ����Ⱦɫ��־����־��¼����С�ڵ��ڸü���Ķ���¼������*/
    int8_t  m_cTraceLogLocalLevel;
    /* Ⱦɫ��־���ĵ���־��¼����С�ڵ��ڸü���Ķ����͵�Ⱦɫ��־����, һ������Ϊ_LOGSYS_WARNING_*/
    int8_t m_cTraceLogMsgLevel;

    /*����Ⱦɫ����ת��������־����*/
    int8_t m_cColorLogRelayLevel;

    int m_iSocket; 		 //�ý������ݰ�ת��ʱ��ʹ�õ��׽���

    int m_iWaterDestID;  //��ˮ��־�ӿڻ���ID,һ������ΪLS_WATER_DEST_ID
    int m_iColorDestID;  //Ⱦɫ��־�ӿڻ���ID,һ������ΪLS_COLOR_DEST_ID

	int m_bInit;		//api�Ƿ��ʼ����־
}LsClient;


/*
   ʹ��APIʱ��ע��:
   һ��LsCheckWriteWL()��LsWaterLog()������ʹ��,Ϊ��ʹ�÷��㣬������LOGSYS_WATER��,��¼��־ʱ��ֱ��ʹ��LOGSYS_WATER()��
   */


/*
   ����˵��:
   ���̳�ʼ��ʱ����LsClientInit(),��ɳ�ʼ���ú���ֻ�ܵ���һ�Σ���Ϊ��
   �ڲ���attach��Ⱦɫ���ù����ڴ���

   ����˵��:
iSvrConfiged:�Ƿ�����Զ�̼�¼��ˮ��־,0:������, 1:����
iColorSvrConfiged:�Ƿ�����Զ�̼�¼Ⱦɫ��־,0:������, 1:����
pstLsInitParam:ָ��LsInitParam�ṹ��,���ڴ�����̳�ʼ���ǿ���ȷ������Ϣ:����Appid,��ʼ���û��Զ����ֶ�1,����ID,���ܴ����ָ��,���������ó���
szLogFilePath:���ش���log�ļ��ľ���·��,���ص���ˮ��־��Ⱦɫ��־����¼�ڸ�·����,���ܴ����ָ��,���������ó���
cWLogLevel:������ˮ��־����־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ���¼������
cWLogMsgLevel:��ˮ��־���ĵ���־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ����͵���ˮ��־����, һ������Ϊ_LC_WARNING_
cTLogLevel:����Ⱦɫ��־����־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ���¼������
cTLogMsgLevel:Ⱦɫ��־���ĵ���־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ����͵�Ⱦɫ��־����, һ������Ϊ_LC_WARNING_
cRelayLogLevel:ת�����ļ�¼����(ȡֵ��Χ0-5,��ο�"��־������"),С�ڵ��ڸü�����Ⱦɫ��������,��ת����ָ����ip:port
iLogSize:���ش���log�ļ������ļ��Ĵ�С
iLogNum:���ش���log�ļ���������
iWaterDestID: ��ˮ��־��Ҫ���͵�����־����ID����ˮ��־���Ŀ����ж���,ÿ��һ��ID��
iColorDestID: Ⱦɫ��־��Ҫ���͵�����־����ID��Ⱦɫ��־���Ŀ����ж���,ÿ��һ��ID��
#define LS_WATER_DEST_ID	1		//��ˮ��־����ID��1Ϊ����ϵͳID�����ҵ����Ҫ�����һ����־���Ĵ洢��ˮ��Ϣ����ô�������������ID
#define LS_COLOR_DEST_ID	2		//Ⱦɫ��־����ID�� 2Ϊ����ϵͳID��������ҵ����Ҫ�����һ�״洢Ⱦɫ��Ϣ����ô�������������ID


��������ֵ:
-30:Ⱦɫ���ù����ڴ�attachʧ��,�����ô��󷵻�ֵ,���̿��Բ��˳�,���޷�ʹ��Ⱦɫ���ù��ܡ�������û�а�װȾɫ���ÿͻ��˵���
-40:��־���Ľӿڻ�(logsy_if)���ù����ڴ�attachʧ��,�����ô��󷵻�ֵ,���̿��Բ��˳�,���ǲ��ܽ���־ͨ�����緢����־���ġ�������û�а�װȾɫ���ÿͻ��˵���
<0 �Ҳ�����-30��-40:ʧ��,���Ӧ�˳�
=0:�ɹ�
*/
int LsClientInit(int iSvrConfiged, int iColorSvrConfiged,  
        LsInitParam *pstLsInitParam , char * szLogFilePath, 
        int8_t cWLogLevel, int8_t cWLogMsgLevel,  int8_t cTLogLevel, int8_t cTLogMsgLevel,  int8_t cRelayLogLevel, 
        int iLogSize, int iLogNum, int iWaterDestID, int iColorDestID);





/*
   ����˵��:
   �ڽ������й����п�ͨ�������źŵ���LsReloadLogCfg()������ĳЩͳһ��־������ص�����

   ����˵��:
iSvrConfiged:�Ƿ�����Զ�̼�¼��ˮ��־,0:������, 1:����
iColorSvrConfiged:�Ƿ�����Զ�̼�¼Ⱦɫ��־,0:������, 1:����
cWLogLevel:������ˮ��־����־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ���¼������
cWLogMsgLevel:��ˮ��־���ĵ���־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ����͵���ˮ��־����, һ������Ϊ_LC_WARNING_
cTLogLevel:����Ⱦɫ��־����־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ���¼������
cTLogMsgLevel:Ⱦɫ��־���ĵ���־��¼����(ȡֵ��Χ0-5,��ο�"��־������")��С�ڵ��ڸü���Ķ����͵�Ⱦɫ��־����, һ������Ϊ_LC_WARNING_
cRelayLogLevel:ת�����ļ�¼����(ȡֵ��Χ0-5,��ο�"��־������"),С�ڵ��ڸü�����Ⱦɫ��������,��ת����ָ����ip:port
iWaterDestID: ��ˮ��־��Ҫ���͵�����־����ID����ˮ��־���Ŀ����ж���,ÿ��һ��ID��
iColorDestID: Ⱦɫ��־��Ҫ���͵�����־����ID��Ⱦɫ��־���Ŀ����ж���,ÿ��һ��ID��
#define LS_WATER_DEST_ID	1		//��ˮ��־����ID��1Ϊ����ϵͳID�����ҵ����Ҫ�����һ����־���Ĵ洢��ˮ��Ϣ����ô�������������ID
#define LS_COLOR_DEST_ID	2		//Ⱦɫ��־����ID�� 2Ϊ����ϵͳID��������ҵ����Ҫ�����һ�״洢Ⱦɫ��Ϣ����ô�������������ID

��������ֵ:
=0:�ɹ�,�ú���һ����ִ�гɹ�
*/
int LsReloadLogCfg(int iSvrConfiged, int iColorSvrConfiged,  
        int8_t cWLogLevel, int8_t cWLogMsgLevel,  int8_t cTLogLevel, int8_t cTLogMsgLevel, 
        int8_t cRelayLogLevel,  int iWaterDestID, int iColorDestID);




/*
   ����˵��:
           ��¼��־֮ǰ�ȵ���LsClientBegin(),��APIҪ����һ��LsPkgParamָ��,LsPkgParam
   ���ֶε���ϸ���ο����ݽṹ�Ķ��壬Ϊ�˱��ڶ�λ���⣬Ҫ������ֶξ�����д������������
   ddwUin��dwCmdһ��Ҫ��д��ȷ,��������ddwUinΪ0, ����־���ĻὫΪ0��ddwUin �滻Ϊһ��
   ���ֵ
            ���ر�ע��:��ddwUin��,LsPkgParam�е��ֶ�ֻ��¼����־���ĵ����ݿ���,��������־���ĵ�
    ���ݿ��а��ֶν�������,�����ڱ�����־�ļ��н��м�¼��
           ÿ�յ�һ��������󣬸����հ�socket�����Զ˵�IP��ַ����������ͷ(��Щϵͳ������Ҫ��������
   ����)��ɵõ�LsPkgParam�ṹ�еĸ����ֶΣ�Ȼ�����LsClientBegin()������ĳЩϵͳû��uin��
   ����,�����ddwUin���0����


   ����˵��:
pstLsPkgParam:ָ��һ��LsPkgParam�ṹ��,�ýṹ��ĸ��ֶ�����д����,�����ǿ�ָ��,����
����ִ��ʧ��


��������ֵ:
<0: ʧ��
=0:�ɹ�
*/
int LsPreSetParam(LsPkgParam *pstLsPkgParam);




/*
   ����˵��:
   �޸�LsPkgParam�ṹ�е�dwErrId, dwUserDef1-4, LsPkgParam�ṹ�е�dwErrId, dwUserDef1-4�Ƚ����⣬
   һ�δ�������У�������Ҫ����޸�dwErrId, dwUserDef1-4�е�ĳһ����ĳ�����ֶ�,�ɵ���
   LsSetParam���������޸�

   ����˵��:
dwType: ȡֵ��ΧΪ���еĺ궨��
#define	_LC_ERR_RET_ID_		1  //�޸�LsPkgParam��dwErrId
#define	_LC_USER_DEF1_		2  //�޸�LsPkgParam��dwUserDef1
#define	_LC_USER_DEF2_		3  //�޸�LsPkgParam��dwUserDef2
#define	_LC_USER_DEF3_		4  //�޸�LsPkgParam��dwUserDef3
#define	_LC_USER_DEF4_		5  //�޸�LsPkgParam��dwUserDef4
dwVaule: Ҫ���õ�ֵ

��������ֵ:
=0:�ɹ�,�ú���һ����ִ�гɹ�
*/
int LsSetParam(uint32_t dwType, uint32_t dwVaule);



/*
   ����˵��:
   ����LsCheckWriteWL()�жϷ�Ҫ��¼��־

   һ��LsCheckWriteWL()��LsWaterLog()������ʹ��,Ϊ��ʹ�÷��㣬������LOGSYS_WATER��,��¼��־ʱ��ֱ��ʹ��LOGSYS_WATER()��

   ����˵��:
iLogLevel: Ҫ��¼��־�ļ��� (ȡֵ��Χ0-5,��ο�"��־������")

��������ֵ:
=0
*/
int LsCheckWriteWL(int iLogLevel);





/*
   ����˵��:
   ����LsWaterLog()������־��¼����


   һ��LsCheckWriteWL()��LsWaterLog()������ʹ��,Ϊ��ʹ�÷��㣬������LOGSYS_WATER��,��¼��־ʱ��ֱ��ʹ��LOGSYS_WATER()��

   ����˵��:
szFile:Դ�ļ���
iLine:Դ��������ڵ���
szFunction:Դ��������ڵĺ���
iLogLevel: Ҫ��¼��־�ļ��� (ȡֵ��Χ0-5,��ο�"��־������")
sFormat:��־�ĸ�ʽ
��������ֵ:
=0
*/
int LsWaterLog(const char * szFile, int iLine, const char * szFunction, int iLogLevel,  const char *sFormat, ...)
__attribute__ ((format(printf,5,6)));

/*
   ����˵��:
   ����LsWaterLogStr()������־��¼����, ����LOGSYS_WATER_STR()��

   ����˵��:
szFile:Դ�ļ���
iLine:Դ��������ڵ���
szFunction:Դ��������ڵĺ���
iLogLevel: Ҫ��¼��־�ļ��� (ȡֵ��Χ0-5,��ο�"��־������")
szLogStr:��־�ַ���
��������ֵ:
=0
*/
int LsWaterLogStr(const char * szFile, int iLine, const char * szFunction, int iLogLevel, char *szLogStr);

/*
   ����˵��:
   ת��������������ת������Ⱦɫ���ý�ĳЩuin�������ƥ��������ת����ָ����ip:port

   ����˵��:
iLogLevel: Ҫת�����ļ��� (ȡֵ��Χ1-5,��ο�"��־������")
sPkg:Ҫת���������,������iPkgLenָ��,�����ǿ�ָ��
iPkgLen:sPkg�ĳ���,ȡֵ��Χ(0-64KB)


��������ֵ:
=0:������ת������, δת��
=1:����ת������, ��ת��
=-10:�����������
<0������ֵ:�ڲ�ϵͳ����
*/
int LsColorRelayPkg(int iLogLevel, void *sPkg, int iPkgLen);

/*
   ����˵��:
   ת��������������ת������Ⱦɫ���ý�ĳЩuin�������ƥ��������ת����ָ����ip:port

   ����˵��:
   	iSock: ����ת������socket
	iLogLevel: Ҫת�����ļ��� (ȡֵ��Χ1-5,��ο�"��־������")
	sPkg:Ҫת���������,������iPkgLenָ��,�����ǿ�ָ��
	iPkgLen:sPkg�ĳ���,ȡֵ��Χ(0-64KB)


��������ֵ:
	=0:������ת������, δת��
	=1:����ת������, ��ת��
	=-1:iSockС��0
	=-10:�����������
	<0������ֵ:�ڲ�ϵͳ����
*/
int LsColorRelayPkgWithSock(int iSock, int iLogLevel,  void *sPkg, int iPkgLen);

/*���ô�����Ϣ*/
int LsSetRetErrMsg(char * szErrMsg);

/*���������Ϣ*/
int LsClearRetErrMsg();

/*��¼ͳ����־*/
int LsStaticsLog(const char *sFormat, ...);

/*�Ƿ��ǹ�˾��غ���*/
/*
��������ֵ��
	=1���ǹ�˾��غ���
	=0�����ǹ�˾��غ���
	<0������
*/
int LsIsCompanyUin(uint64_t ddwUin);


/*
   APIʹ��˵��
   1.�ڽ��̳�ʼ��ʱ����LsClientInit()
   2.ÿ�յ�һ���������ʼһ���µĴ�������,��Ҫ����LsPreSetParam()
   3.ʹ��LOGSYS_WATER���¼��־
   4.�ڽ�����Ӫ������,���Ҫ�޸�ĳЩ������,�ɵ���LsReloadLogCfg()
   5.������ֱ���޸�g_stLsClient�ĳ�Ա����
   6.���Ҫ��Ⱦɫ�������е������ת����ָ����ip:port,����LsColorRelayPkg()����
   ���жϷ���ֵ�Ƿ�Ϊ1,�������ֵΪ1,����ת���ɹ�,����ת��ʧ��
   7.������Ϣ��ο���ͳһ��־ϵͳʹ��˵��.doc��
*/

extern LsClient g_stLsClient;

#define LOGSYS_WATER(iLogLevel, fmt, args...) do { \
    if (LsCheckWriteWL(iLogLevel)) \
    LsWaterLog(__FILE__ , __LINE__ , __FUNCTION__, iLogLevel, fmt, ## args); }  while (0)

#define LOGSYS_WATER_STR(iLogLevel, szLogStr) do { \
	if (LsCheckWriteWL(iLogLevel)) \
		LsWaterLogStr(__FILE__ , __LINE__ , __FUNCTION__, iLogLevel, szLogStr); }  while (0)

#define LOGSYS_BUF(iLogLevel, title, buffer, len) do { \
    if (LsCheckWriteWL(iLogLevel)) \
    { \
        LsWaterLog(__FILE__ , __LINE__ , __FUNCTION__, iLogLevel, "%s\n", title);   \
    LsWaterLog(__FILE__ , __LINE__ , __FUNCTION__, iLogLevel, DumpPackage(buffer, len));     \
} \
}while (0)


/********************************��ӶԶ�ҵ��֧�ֺ���µļ�¼��־�ĺ���*********************************/
/*
!*����ҵ�����־�����Ƿ�ʹ��Զ����־����Ϣ��
!*��Ҫ����ҵ���������ڼ��Լ��޸�����
!*/
typedef struct
{
	int8_t cWLogLevel;    /*������ˮ��־����*/
	int8_t cWLogMsgLevel; /*Զ����ˮ��־����*/
	int8_t cTLogLevel;
	int8_t cTLogMsgLevel;
	int8_t cRelayLogLevel;
}LsLogLevelConf;
	


/************************************************************************************************************/
/*                                          ���ÿͻ���ʼ����������ز���                                    */
/************************************************************************************************************/
/*
!*brief : �û��Զ����ֶΣ����ڳ�ʼ��ʱ���ݸ�LsLogFsOpen��������Ҫ�������Ƿ���Զ����־��Զ����־ϵͳid����Ϣ��
!*/
typedef struct 
{
	int   iSvrConfiged;
	int   iColorSvrConfiged;
	int   m_iWaterDestID;
	int   m_iColorDestID;
    uint32_t dwUserDef1;       //��ʼ���û��Զ����ֶ�1���ɵ������Լ���д���Լ�����
    uint32_t dwProcId;
	uint16_t wMsgPort;			//��ָ������Զ����־���󶨵Ķ˿ڣ�Ϊ0������˿�
	uint16_t wRelayPort;		//��ָ��ת�������󶨵Ķ˿ڣ�Ϊ0������˿�
	uint16_t cAgentLocalLog;    //�Ƿ���Agent�������¼����log��ҵ����̲��ټ�¼
}LOGSYS_CLIENT_INFO;


/*
!*brief : ��ʼ��ҵ����ص���Ϣ
!*Ĭ�Ͽ���Զ����ˮ��־��Զ��Ⱦɫ��־(������������и�ҵ�����־������ص����ã���ʹ���������ĵ���־��������ʹ��Ĭ��
!*����־����:Զ����־ʹ��LC_WARNING��������ˮ����ΪLC_WARNING���رձ���Ⱦɫ����
!*Ĭ�Ͽ�����agent��д������־�Ĺ���
!*
!*ע��: ������е�ֵ��ʹ��Ĭ��ֵ��Զ����־���Ļ�ʹ���������ĵ����ã������Ҫ��װ���µ�agent������
!*��Ҫ����LsLogFsClientInfoSetLogDestID��������Զ����־���ĵ�id��
!*/

int LsLogFsClientInfoInit(LOGSYS_CLIENT_INFO *pstClientInfo);

/*
!*���ý��̺�,����api�Լ�д������־ʱ���������ֲ�ͬ�Ľ���
!*/
int LsLogFsClientInfoSetProcId(LOGSYS_CLIENT_INFO *pstClientInfo, uint32_t value);

/*
!*���ó�ʼ��ʱ��userdef�
!*/
int LsLogFsClientInfoSetInitUserDef(LOGSYS_CLIENT_INFO *pstClientInfo, uint32_t value);

/*
!*�����Ƿ�ʹ��Զ��Ⱦɫ��־���ģ�value=0��ʾ�������ʾ��
!*/
int LsLogFsClientInfoSetUseColorSvr(LOGSYS_CLIENT_INFO *pstClientInfo, int value);

/*
!*�����Ƿ��¼Զ����ˮ��־��value=0��ʾ�������ʾ��
!*/
int LsLogFsClientInfoSetUseWaterSvr(LOGSYS_CLIENT_INFO *pstClientInfo, int value);

/*
!*�������ݰ�ת��ʱ��ʹ�õĶ˿ں�
!*/
int LsLogFsClientInfoSetRelayPort(LOGSYS_CLIENT_INFO *pstClientInfo, uint16_t value);

/*����Զ��Ⱦɫ���õ�ϵͳid��Զ����ˮ��־��ϵͳid*/
int LsLogFsClientInfoSetLogDestID(LOGSYS_CLIENT_INFO *pstClientInfo, int iWaterDestID, int iColorDestID);




/*
!*��ҵ�񷽳�ʼ��ʱ���ã�����ҪЯ���κ�����
!*brief :ֻʹ��ҵ��������ID���г�ʼ���ĺ�����Ĭ�Ͽ���Զ����־���ܣ��رձ�����־����
!*param[in]:szAppName ,ҵ�����ƣ���dwAppId��Ϊ0ʱ����ΪNULL���255
!*param[in]:szUserName,ҵ���������ƣ���dwAppId��Ϊ0ʱ����ΪNULL���Ϊ1024
!*param[in]:dwAppId   ,ҵ��ID���粻֪����0
!*retval >=0 :һ����ȷ�ľ����other��ʼ������
!*������־��ص���Ϣ�������˱�����־���ļ�����ÿ����־�ļ��Ĵ�С����־�ļ��ľ���·��
!*/
	int LsLogFsOpenNoConf(const char * szAppName, const char * szUserName, const uint32_t dwAppId,
								LsLogFileInfo *pstLsLogFileInfo);


/*
!*��ҵ�񷽵��ã���д��־֮ǰ��Ҫ���øú���
!*brief    :����ҵ�����ƣ���ȡһ����Ӧ�ľ�� 
!*param[in]:szAppName		,ҵ�����ƣ���dwAppId��Ϊ0ʱ����ΪNULL���255
!*param[in]:szUserName		 ,ҵ���������ƣ���dwAppId��Ϊ0ʱ����ΪNULL���Ϊ1024
!*param[in]:iSvrConfiged	  ,�Ƿ�ʹ��Զ����ˮ��־����
!*param[in]:iColorSvrConfiged ,�Ƿ�ʹ��Զ��Ⱦɫ��־����
!*param[in]:pstLsInitParam	  ,ָ��LsInitParam�ṹ��,���ڴ�����̳�ʼ���ǿ���ȷ������Ϣ:����Appid,��ʼ���û��Զ����ֶ�1,����ID,���ܴ����ָ��,���������ó���
!*param[in]:pstLsLogFileInfo  ,���ڴ���ҵ��ı�����־��ص����ã����籾����־���ļ������ļ���С���ļ�����
!*retval >=0 :һ����ȷ�ľ����other��ʼ������
!*/
/*
!*�Ƚ�һ��ȷ�����Զ�ע����������ȷ�ϸú���
!*/
int LsLogFsOpen(const LOGSYS_CLIENT_INFO *pstClientInfo,
                         const char * szAppName, const char * szUserName,  const uint32_t dwAppId,
                         LsLogFileInfo *pstLsLogFileInfo);

/*
!*
!*brief : �ر�һ�����
!*param[in] iFd : Ҫ�رյ�log file������
!*retval 0��ʾ��ȷ��other��ʼ������
!*/
int LsLogFsClose(const int iFd);

/*
!*
!*brief : ��һ�����������д����־��Ϣ�����Ȼ�ȡ��FD��Ӧ��ʵ����Ȼ��ʹ�����б����������Ϣ��Ϊҵ��д��־��Ϣ
!*/
int LsLogFsWrite(const int iFd, const char * szFile, int iLine, 
					 const char * szFunction, int iLogLevel, const char *sFormat, ...)
__attribute__ ((format(printf,6,7)));

/*
!*brief : ҵ���ṩva_list��Ϊ������д��־����
!*param[in] iFd : ��ʼ��ʱ���ص��ļ�������
!*param[in] va	: Ҫд���valist����
!*/
int LsLogFsWriteVA(const int iFd, const char * szFile, int iLine, 
						const char * szFunction, int iLogLevel, const char *sFormat, va_list va);


/*
!*brief : ת�����ݰ��ĺ���
!*param[in] iFd : �ɳ�ʼ���������ص���ҵ���Ӧ�ľ��
!*param[in] iLogLevel : ����
!*param[in] sPkg : ��Ҫת�������ݰ�
!*param[in] iPkgLen : ��Ҫת�������ݰ��ĳ���
!*retval : 1��ʾ�ɹ��� other ��ʾʧ��
!*/
int LsLogFsRelayPkg(const int iFd, const int iLogLevel,  const void *sPkg, const int iPkgLen);

/*
!*brief : ���õ�ǰҵ�����־��Ϣ������userdef��uin�ȵ�
!*param[in] : iFd Ҫ���õ�ҵ������Ӧ���ļ�������
!*param[in] : pstLsPktParam Ҫ���õ����ݰ��Ĳ�������ָ�벻��Ϊ��
!*retval	: 0 �ɹ��� other ʧ�� 
!*/
int LsLogFsPreSetPktParam(int iFd, LsPkgParam *pstLsPktParam);

/*
!*brief : ���������������ݰ���������Щ����������ƥ��Ⱦɫ����ʱʹ��
!*param[in] : iFd ,open�������ظ�ҵ���������
!*param[in] : dwType , Ҫ���õĲ���������
!*param[in] : dwValue, Ҫ���õĲ�����ֵ
!*retval	: 0 �ɹ��� other ʧ�� 
!*/
int LsLogFsSetPktParam(int iFd, uint32_t dwType, uint32_t dwVaule);

/*
!*brief : ��ȡһ��ҵ�����־�����������Ϣ
!*param[in] : iFd ,��ʼ��ʱ���ظ�ҵ�񷽵��ļ�������
!*param[out]: pstLsLogLevelConf, ҵ�����־���������
!*/
int LsLogGetConf(int iFd, LsLogLevelConf *pstLsLogLevelConf);

/*
!*brief : ������־�����������Ϣ
!*param[in] : iFd ,��ʼ��ʱ���ظ�ҵ�񷽵��ļ�������
!*param[in] : pstLsLogLevelConf , Ҫ���õ��µ���־�������Ϣ��
!retval 	: 0 LOGSYS_EOK �ɹ�������ֵ��ʾʧ��
!*ps : ��������øú������������ĵ�ҵ����־�������޸�ʱ���������ĵ�����Ϊ׼
!*/
int LsLogSetConf(int iFd, LsLogLevelConf *pstLsLogLevelConf);


/*
!*brief :�����ж�һ����־����־�����Ƿ����㷢�͵�����,����Ⱦɫ���ý���ƥ��
!*param[in] iFd: Ҫ�жϵ�ҵ���Ӧ���ļ�������
!*param[in] iLogLevel ��־����
!*retval 1 ����־�������־��Ҫ����һ������0 �ü������־����Ҫ����һ������other ����
!*/
int LsLogFsCheckLevel(const int iFd, const int iLogLevel);


/*
   APIʹ��˵��
   1.�ڽ��̳�ʼ��ʱ����LsLogFsOpen ����LsLogFsOpenNoCfg������ȡһ�����
   2.ÿ�յ�һ���������ʼһ���µĴ�������,��Ҫ����LsLogFsPreSetPktParam��������صĲ���
   3.ʹ��LOGSYS_WATER_V2���¼��־
   4.�ڽ�����Ӫ������,���Ҫ�޸�ĳЩ������,�ɵ���LsLogSetConf()�������µ����ã�Ҳ����ʹ��LsLogGetConf����ȡ�ɵ�����
   5.������ֱ���޸�g_stLsClient�ĳ�Ա����
   6.���Ҫ��Ⱦɫ�������е������ת����ָ����ip:port,����LsLogFsRelayPkg()����
   ���жϷ���ֵ�Ƿ�Ϊ1,�������ֵΪ1,����ת���ɹ�,����ת��ʧ��
   7.������Ϣ��ο���ͳһ��־ϵͳʹ��˵��.doc��
*/

#define LOGSYS_WATER_V2(iFd, iLogLevel, fmt, args...) do{\
	if (LsLogFsCheckLevel(iFd, iLogLevel)) \
	LsLogFsWrite(iFd, __FILE__, __LINE__, __FUNCTION__, iLogLevel, fmt, ##args);\
}while(0)

/*��valistΪ����*/
#define LOGSYS_WATER_VA_LIST(iFd, iLogLevel, fmt, ap) do{\
	if (LsLogFsCheckLevel(iFd, iLogLevel)) \
	LsLogFsWriteVA(iFd, __FILE__, __LINE__, __FUNCTION__, iLogLevel, fmt, ap);\
}while(0)

#define LOGSYS_WATER_STR_V2(iFd, iLogLevel, szLogStr) do{\
	if (LsLogFsCheckLevel(iFd, iLogLevel)) \
	 LsLogFsWrite(iFd, __FILE__ , __LINE__ , __FUNCTION__, iLogLevel, "%s", szLogStr);\
}while(0)

#define LOGSYS_BUF_V2(iFd, iLogLevel, title, buffer, len) do { \
	if (LsLogFsCheckLevel(iFd, iLogLevel)) {\
	LsLogFsWrite(iFd, __FILE__ , __LINE__ , __FUNCTION__, iLogLevel, "%s\n", title);   \
    LsLogFsWrite(iFd, __FILE__ , __LINE__ , __FUNCTION__, iLogLevel, DumpPackage(buffer, len));    } \
}while (0)

/*
   ����˵��:
   ת��������������ת������Ⱦɫ���ý�ĳЩuin�������ƥ��������ת����ָ����ip:port

   ����˵��:
iLogLevel: Ҫת�����ļ��� (ȡֵ��Χ1-5,��ο�"��־������")
sPkg:Ҫת���������,������iPkgLenָ��,�����ǿ�ָ��
iPkgLen:sPkg�ĳ���,ȡֵ��Χ(0-64KB)


��������ֵ:
=0:������ת������, δת��
=1:����ת������, ��ת��
=-10:�����������
<0������ֵ:�ڲ�ϵͳ����
*/

/*********************************************end*******************************************************/
}

#endif

