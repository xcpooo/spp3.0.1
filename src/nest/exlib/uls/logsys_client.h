/*********************************************************************
 * ÈÕÖ¾Àà£¬·ÖÀà¼ÇÂ¼ÈÕÖ¾
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

/* ÈÕÖ¾¼¶±ğ¶¨Òå*/
#define _LC_ERROR_ 		1	//ÑÏÖØ´íÎó
#define _LC_WARNING_ 	2 	//¾¯¸æ,±È½ÏÑÏÖØ£¬ĞèÒª·¢µ½ÊÖ»ú
#define _LC_NOTICE_ 		3 	//ĞèÒª×¢ÒâµÄÎÊÌâ£¬¿ÉÄÜÊÇ·Ç·¨µÄ°üÖ®Àà£¬²»ĞèÒª·¢ËÍµ½ÊÖ»ú£¬ÒòÎª¿ÉÄÜºÜ¶à
#define _LC_INFO_ 			4	//Á÷Ë®
#define _LC_DEBUG_ 		5	//debug


#define LOGSYS_MAX_LOGD_IF_SVR			300		//logsys_if·şÎñÆ÷µÄ×î´ó¸öÊı
#define LOGSYS_LOGD_IF_IP_LEN			30           //logsys_if·şÎñÆ÷µÄIP×î´ó³¤¶È

#define LOGSYS_SENDTO_WATER			1		//·¢ËÍÁ÷Ë®ÈÕÖ¾
#define LOGSYS_SENDTO_COLOR			2	        //·¢ËÍÈ¾É«ÈÕÖ¾

#define LS_INVALID_DEST_ID 0 //·Ç·¨µÄÈÕÖ¾½Ó¿Ú»úrole
#define LS_WATER_DEST_ID				1		//Í¨ÓÃÁ÷Ë®ÈÕÖ¾½Ó¿Ú»úÔÚÅäÖÃÖĞĞÄÖĞµÄrole
#define LS_COLOR_DEST_ID				2		//Í¨ÓÃÈ¾É«ÈÕÖ¾½Ó¿Ú»úÔÚÅäÖÃÖĞĞÄÖĞµÄrole

#define	_LC_ERR_RET_ID_		1  //ĞŞ¸ÄLsPkgParamµÄdwErrId
#define	_LC_USER_DEF1_		2  //ĞŞ¸ÄLsPkgParamµÄdwUserDef1
#define	_LC_USER_DEF2_		3  //ĞŞ¸ÄLsPkgParamµÄdwUserDef2
#define	_LC_USER_DEF3_		4  //ĞŞ¸ÄLsPkgParamµÄdwUserDef3
#define	_LC_USER_DEF4_		5  //ĞŞ¸ÄLsPkgParamµÄdwUserDef4

#define LOGSYS_MAX_CMD_STR_LEN 64

#define LOGSYS_STATICS(fmt, args...) do { \
    LsStaticsLog(fmt, ## args); }  while (0)

#define SET_LOGSYS_ERRMSG(szErrMsg) do { \
    LsSetRetErrMsg(szErrMsg); }  while (0)

#define CLEAR_LOGSYS_ERRMSG() do { \
    LsClearRetErrMsg(); }  while (0)

/*
   ÒªÊ¹ÓÃÒÔÏÂĞ­Òé´òÓ¡Ïà¹ØµÄºê£¬±ØĞëÏÈÔÚ°üº¬±¾Í·ÎÄ¼şÖ®Ç°°üº¬Ğ­Òé¶ÔÓ¦µÄdisplayÍ·ÎÄ¼ş£¬Èç"display_XXX.h"
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
    uint32_t dwAppId; 			//ÒµÎñµÄAppId,ÓÉÏµÍ³¹ÜÀíÔ±·ÖÅä,±ØÌî×Ö¶Î
    uint32_t dwUserDef1;       //³õÊ¼»¯ÓÃ»§×Ô¶¨Òå×Ö¶Î1£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ
    uint32_t dwProcId;
	uint16_t wMsgPort;			//¿ÉÖ¸¶¨·¢ËÍÔ¶³ÌÈÕÖ¾Ëù°ó¶¨µÄ¶Ë¿Ú£¬Îª0ÔòËæ»ú¶Ë¿Ú
	uint16_t wRelayPort;		//¿ÉÖ¸¶¨×ª·¢°üËù°ó¶¨µÄ¶Ë¿Ú£¬Îª0ÔòËæ»ú¶Ë¿Ú
	uint16_t cAgentLocalLog;    //ÊÇ·ñÓÉAgentÀ´¸ºÔğ¼ÇÂ¼±¾µØlog£¬ÒµÎñ½ø³Ì²»ÔÙ¼ÇÂ¼
}LsInitParam;   //½ø³Ì³õÊ¼»¯Ê±¿ÉÈ·¶¨µÄĞÅÏ¢

typedef struct  
{
    uint64_t ddwUin;   //64Î»uin£¬±ØÌî×Ö¶Î,Í¬connµÄqwUin
    uint32_t dwClientIp; //´¥·¢Ç°¶Ë·şÎñµÄÓÃ»§IP£¬Ò»°ãÊÇÓÃ»§ËùÓÃ»úÆ÷µÄIP,ÍøÂçĞò£¬Í¬connµÄdwClientIP
    uint8_t  sClientIpV6[16];  //´¥·¢Ç°¶Ë·şÎñµÄÓÃ»§IP£¬Ò»°ãÊÇÓÃ»§ËùÓÃ»úÆ÷µÄIP,ipv6 ¸ñÊ½
    uint32_t dwServiceIp;   //Ç°¶ËÓ¦ÓÃÖ÷»úIP,conn ipÈÏÎªÊÇÇ°¶ËÓ¦ÓÃÖ÷»úIP,ÍøÂçĞò £¬Í¬connµÄdwServiceIp
    uint32_t dwClientVersion;  //¿Í»§¶Ë°æ±¾ºÅ,Í¬connµÄwVersion
    uint32_t dwCmd;      //ÃüÁîºÅ£¬Í¬connµÄwCmd
    uint32_t dwSubCmd;   //×ÓÃüÁîºÅ£¬Í¬connµÄdwSubCmd
    int32_t dwErrId;   //´¦Àí¹ı³ÌÖĞµÄ´íÎó·µ»ØÂë£¬Í¬connµÄwErrNo£¬Ã¿´Îµ÷ÓÃ¼ÇÂ¼ÈÕÖ¾µÄ apiÊ±¶¼Òª´«Èë¸Ã²ÎÊı  
    uint32_t dwUserDef1;  //ÓÃ»§×Ô¶¨Òå×Ö¶Î1£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ£¬connµÄcPkgType£¬dwLoginTime£¬dwLogOutTime¿ÉÊ¹ÓÃ×Ô¶¨Òå×Ö¶Î¼ÇÂ¼²¢Ë÷Òı
    uint32_t dwUserDef2;  //ÓÃ»§×Ô¶¨Òå×Ö¶Î2£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ
    uint32_t dwUserDef3;  //ÓÃ»§×Ô¶¨Òå×Ö¶Î3£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ
    uint32_t dwUserDef4;  //ÓÃ»§×Ô¶¨Òå×Ö¶Î4£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ  
    //char szCmdStr[LOGSYS_MAX_CMD_STR_LEN];
}LsPkgParam;   //Ò»´ÎÇëÇó°ü´¦Àí¿ªÊ¼Ê±²ÅÄÜÈ·¶¨µÄĞÅÏ¢

typedef struct
{
	char * szLogFilePath;  /*±¾µØÈÕÖ¾ÎÄ¼şÃû£¬³¤¶È²»ÄÜ³¬¹ı55*/
	int iLogSize;
	int iLogNum;
	uint32_t dwMaxBufferBytes; /*×î´óÄÜ»º´æµÄ±¾µØ×Ö½ÚÊı*/
	uint32_t dwMaxBufferTime;  /*×î´óÄÜ»º´æµÄÊ±¼ä*/
}LsLogFileInfo;

typedef struct
{
    /* ´íÎóĞÅÏ¢, ±£Áô×îºóÒ»´Îµ÷ÓÃµÄ´íÎóĞÅÏ¢£¬¿ÉÒÔÓÃÓÚÏêÏ¸µÄ·µ»Ø*/	
    char m_szErrMsg[512];
    LS_LogFile  m_stWaterLog;

    /* Á÷Ë®ÈÕÖ¾·şÎñÆ÷¶ËÅäÖÃĞÅÏ¢*/
    int m_iSvrConfiged;     //0:²»ÆôÓÃÔ¶³Ì¼ÇÂ¼Á÷Ë®ÈÕÖ¾, ·Ç0:ÆôÓÃÔ¶³Ì¼ÇÂ¼Á÷Ë®ÈÕÖ¾

    /* È¾É«ÈÕÖ¾·şÎñÆ÷¶ËÅäÖÃĞÅÏ¢*/
    int m_iColorSvrConfiged;   //0:²»ÆôÓÃÔ¶³Ì¼ÇÂ¼È¾É«ÈÕÖ¾, ·Ç0:ÆôÓÃÔ¶³Ì¼ÇÂ¼È¾É«ÈÕÖ¾

    /* ÈÕÖ¾»ù±¾ĞÅÏ¢ */ 
    LsInitParam m_stLsInitParam;
    uint64_t m_ddwRandUin;

    /* ±¾µØÁ÷Ë®ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ*/
    int8_t m_cWaterLogLocalLevel;
    /* Á÷Ë®ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½Á÷Ë®ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LOGSYS_WARNING_*/
    int8_t m_cWaterLogMsgLevel;

    /*Í³¼ÆÈÕÖ¾*/
    LS_LogFile  m_stStaticsLog;

    /* debug ÈÕÖ¾*/
    LS_LogFile  m_stTraceLog;
    /* ±¾µØÈ¾É«ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ*/
    int8_t  m_cTraceLogLocalLevel;
    /* È¾É«ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½È¾É«ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LOGSYS_WARNING_*/
    int8_t m_cTraceLogMsgLevel;

    /*¸ù¾İÈ¾É«ÅäÖÃ×ª·¢°üµÄÈÕÖ¾¼¶±ğ*/
    int8_t m_cColorLogRelayLevel;

    int m_iSocket; 		 //ÓÃ½øĞĞÊı¾İ°ü×ª·¢Ê±ËùÊ¹ÓÃµÄÌ×½Ó×Ö

    int m_iWaterDestID;  //Á÷Ë®ÈÕÖ¾½Ó¿Ú»úµÄID,Ò»°ãÉèÖÃÎªLS_WATER_DEST_ID
    int m_iColorDestID;  //È¾É«ÈÕÖ¾½Ó¿Ú»úµÄID,Ò»°ãÉèÖÃÎªLS_COLOR_DEST_ID

	int m_bInit;		//apiÊÇ·ñ³õÊ¼»¯±êÖ¾
}LsClient;


/*
   Ê¹ÓÃAPIÊ±Çë×¢Òâ:
   Ò»°ãLsCheckWriteWL()ºÍLsWaterLog()²»µ¥¶ÀÊ¹ÓÃ,ÎªÁËÊ¹ÓÃ·½±ã£¬¶¨ÒåÁËLOGSYS_WATERºê,¼ÇÂ¼ÈÕÖ¾Ê±ÇëÖ±½ÓÊ¹ÓÃLOGSYS_WATER()ºê
   */


/*
   ¹¦ÄÜËµÃ÷:
   ½ø³Ì³õÊ¼»¯Ê±µ÷ÓÃLsClientInit(),Íê³É³õÊ¼»¯¸Ãº¯ÊıÖ»ÄÜµ÷ÓÃÒ»´Î£¬ÒòÎªÆä
   ÄÚ²¿»áattachµ½È¾É«ÅäÖÃ¹²ÏíÄÚ´æÉÏ

   ²ÎÊıËµÃ÷:
iSvrConfiged:ÊÇ·ñÆôÓÃÔ¶³Ì¼ÇÂ¼Á÷Ë®ÈÕÖ¾,0:²»ÆôÓÃ, 1:ÆôÓÃ
iColorSvrConfiged:ÊÇ·ñÆôÓÃÔ¶³Ì¼ÇÂ¼È¾É«ÈÕÖ¾,0:²»ÆôÓÃ, 1:ÆôÓÃ
pstLsInitParam:Ö¸ÏòLsInitParam½á¹¹Ìå,ÓÃÓÚ´«Èë½ø³Ì³õÊ¼»¯ÊÇ¿ÉÒÔÈ·¶¨µÄĞÅÏ¢:±¾»úAppid,³õÊ¼»¯ÓÃ»§×Ô¶¨Òå×Ö¶Î1,½ø³ÌID,²»ÄÜ´«Èë¿ÕÖ¸Õë,·ñÔòº¯Êıµ÷ÓÃ³ö´í
szLogFilePath:±¾µØ´ÅÅÌlogÎÄ¼şµÄ¾ø¶ÔÂ·¾¶,±¾µØµÄÁ÷Ë®ÈÕÖ¾ºÍÈ¾É«ÈÕÖ¾¶¼¼ÇÂ¼ÔÚ¸ÃÂ·¾¶ÏÂ,²»ÄÜ´«Èë¿ÕÖ¸Õë,·ñÔòº¯Êıµ÷ÓÃ³ö´í
cWLogLevel:±¾µØÁ÷Ë®ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ
cWLogMsgLevel:Á÷Ë®ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½Á÷Ë®ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LC_WARNING_
cTLogLevel:±¾µØÈ¾É«ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ
cTLogMsgLevel:È¾É«ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½È¾É«ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LC_WARNING_
cRelayLogLevel:×ª·¢°üµÄ¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå"),Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğ²¢ÇÒÈ¾É«ÅäÖÃÃüÖĞ,»á×ª·¢µ½Ö¸¶¨µÄip:port
iLogSize:±¾µØ´ÅÅÌlogÎÄ¼şµ¥¸öÎÄ¼şµÄ´óĞ¡
iLogNum:±¾µØ´ÅÅÌlogÎÄ¼şµÄ×î´ó¸öÊı
iWaterDestID: Á÷Ë®ÈÕÖ¾ĞèÒª·¢ËÍµ½µÄÈÕÖ¾ÖĞĞÄID£¨Á÷Ë®ÈÕÖ¾ÖĞĞÄ¿ÉÄÜÓĞ¶àÌ×,Ã¿Ì×Ò»¸öID£©
iColorDestID: È¾É«ÈÕÖ¾ĞèÒª·¢ËÍµ½µÄÈÕÖ¾ÖĞĞÄID£¨È¾É«ÈÕÖ¾ÖĞĞÄ¿ÉÄÜÓĞ¶àÌ×,Ã¿Ì×Ò»¸öID£©
#define LS_WATER_DEST_ID	1		//Á÷Ë®ÈÕÖ¾ÖĞĞÄID£¬1Îª¹«ÓÃÏµÍ³ID£¬Èç¹ûÒµÎñĞèÒªµ¥¶À´î½¨Ò»Ì×ÈÕÖ¾ÖĞĞÄ´æ´¢Á÷Ë®ĞÅÏ¢£¬ÄÇÃ´¿ÉÒÔÉêÇë·ÖÅäĞÂID
#define LS_COLOR_DEST_ID	2		//È¾É«ÈÕÖ¾ÖĞĞÄID£¬ 2Îª¹«ÓÃÏµÍ³ID£¬Èç¹ûÈç¹ûÒµÎñĞèÒªµ¥¶À´î½¨Ò»Ì×´æ´¢È¾É«ĞÅÏ¢£¬ÄÇÃ´¿ÉÒÔÉêÇë·ÖÅäĞÂID


º¯Êı·µ»ØÖµ:
-30:È¾É«ÅäÖÃ¹²ÏíÄÚ´æattachÊ§°Ü,Óöµ½¸Ã´íÎó·µ»ØÖµ,½ø³Ì¿ÉÒÔ²»ÍË³ö,µ«ÎŞ·¨Ê¹ÓÃÈ¾É«ÅäÖÃ¹¦ÄÜ¡£¿ÉÄÜÊÇÃ»ÓĞ°²×°È¾É«ÅäÖÃ¿Í»§¶Ëµ¼ÖÂ
-40:ÈÕÖ¾ÖĞĞÄ½Ó¿Ú»ú(logsy_if)ÅäÖÃ¹²ÏíÄÚ´æattachÊ§°Ü,Óöµ½¸Ã´íÎó·µ»ØÖµ,½ø³Ì¿ÉÒÔ²»ÍË³ö,µ«ÊÇ²»ÄÜ½«ÈÕÖ¾Í¨¹ıÍøÂç·¢µ½ÈÕÖ¾ÖĞĞÄ¡£¿ÉÄÜÊÇÃ»ÓĞ°²×°È¾É«ÅäÖÃ¿Í»§¶Ëµ¼ÖÂ
<0 ÇÒ²»µÈÓÚ-30ºÍ-40:Ê§°Ü,ø³ÌÓ¦ÍË³ö
=0:³É¹¦
*/
int LsClientInit(int iSvrConfiged, int iColorSvrConfiged,  
        LsInitParam *pstLsInitParam , char * szLogFilePath, 
        int8_t cWLogLevel, int8_t cWLogMsgLevel,  int8_t cTLogLevel, int8_t cTLogMsgLevel,  int8_t cRelayLogLevel, 
        int iLogSize, int iLogNum, int iWaterDestID, int iColorDestID);





/*
   ¹¦ÄÜËµÃ÷:
   ÔÚ½ø³ÌÔËĞĞ¹ı³ÌÖĞ¿ÉÍ¨¹ı·¢ËÍĞÅºÅµ÷ÓÃLsReloadLogCfg()£¬¸üĞÂÄ³Ğ©Í³Ò»ÈÕÖ¾ÖĞĞÄÏà¹ØµÄÅäÖÃ

   ²ÎÊıËµÃ÷:
iSvrConfiged:ÊÇ·ñÆôÓÃÔ¶³Ì¼ÇÂ¼Á÷Ë®ÈÕÖ¾,0:²»ÆôÓÃ, 1:ÆôÓÃ
iColorSvrConfiged:ÊÇ·ñÆôÓÃÔ¶³Ì¼ÇÂ¼È¾É«ÈÕÖ¾,0:²»ÆôÓÃ, 1:ÆôÓÃ
cWLogLevel:±¾µØÁ÷Ë®ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ
cWLogMsgLevel:Á÷Ë®ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½Á÷Ë®ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LC_WARNING_
cTLogLevel:±¾µØÈ¾É«ÈÕÖ¾µÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼¼ÇÂ¼µ½±¾µØ
cTLogMsgLevel:È¾É«ÈÕÖ¾ÖĞĞÄµÄÈÕÖ¾¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")£¬Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğµÄ¶¼·¢ËÍµ½È¾É«ÈÕÖ¾ÖĞĞÄ, Ò»°ãÉèÖÃÎª_LC_WARNING_
cRelayLogLevel:×ª·¢°üµÄ¼ÇÂ¼¼¶±ğ(È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå"),Ğ¡ÓÚµÈÓÚ¸Ã¼¶±ğ²¢ÇÒÈ¾É«ÅäÖÃÃüÖĞ,»á×ª·¢µ½Ö¸¶¨µÄip:port
iWaterDestID: Á÷Ë®ÈÕÖ¾ĞèÒª·¢ËÍµ½µÄÈÕÖ¾ÖĞĞÄID£¨Á÷Ë®ÈÕÖ¾ÖĞĞÄ¿ÉÄÜÓĞ¶àÌ×,Ã¿Ì×Ò»¸öID£©
iColorDestID: È¾É«ÈÕÖ¾ĞèÒª·¢ËÍµ½µÄÈÕÖ¾ÖĞĞÄID£¨È¾É«ÈÕÖ¾ÖĞĞÄ¿ÉÄÜÓĞ¶àÌ×,Ã¿Ì×Ò»¸öID£©
#define LS_WATER_DEST_ID	1		//Á÷Ë®ÈÕÖ¾ÖĞĞÄID£¬1Îª¹«ÓÃÏµÍ³ID£¬Èç¹ûÒµÎñĞèÒªµ¥¶À´î½¨Ò»Ì×ÈÕÖ¾ÖĞĞÄ´æ´¢Á÷Ë®ĞÅÏ¢£¬ÄÇÃ´¿ÉÒÔÉêÇë·ÖÅäĞÂID
#define LS_COLOR_DEST_ID	2		//È¾É«ÈÕÖ¾ÖĞĞÄID£¬ 2Îª¹«ÓÃÏµÍ³ID£¬Èç¹ûÈç¹ûÒµÎñĞèÒªµ¥¶À´î½¨Ò»Ì×´æ´¢È¾É«ĞÅÏ¢£¬ÄÇÃ´¿ÉÒÔÉêÇë·ÖÅäĞÂID

º¯Êı·µ»ØÖµ:
=0:³É¹¦,¸Ãº¯ÊıÒ»¶¨»áÖ´ĞĞ³É¹¦
*/
int LsReloadLogCfg(int iSvrConfiged, int iColorSvrConfiged,  
        int8_t cWLogLevel, int8_t cWLogMsgLevel,  int8_t cTLogLevel, int8_t cTLogMsgLevel, 
        int8_t cRelayLogLevel,  int iWaterDestID, int iColorDestID);




/*
   ¹¦ÄÜËµÃ÷:
           ¼ÇÂ¼ÈÕÖ¾Ö®Ç°ÏÈµ÷ÓÃLsClientBegin(),¸ÃAPIÒªÇó´«ÈëÒ»¸öLsPkgParamÖ¸Õë,LsPkgParam
   ¸÷×Ö¶ÎµÄÏêÏ¸º¬²Î¿¼Êı¾İ½á¹¹µÄ¶¨Òå£¬ÎªÁË±ãÓÚ¶¨Î»ÎÊÌâ£¬ÒªÇó¸÷¸ö×Ö¶Î¾¡Á¿ÌîĞ´ÍêÕû£¬ÓÈÆäÊÇ
   ddwUinºÍdwCmdÒ»¶¨ÒªÌîĞ´ÕıÈ·,Èç¹û´«ÈëµÄddwUinÎª0, ÔòÈÕÖ¾ÖĞĞÄ»á½«Îª0µÄddwUin Ìæ»»ÎªÒ»¸ö
   Ëæ»úÖµ
            ÇëÌØ±ğ×¢Òâ:³ıddwUinÍâ,LsPkgParamÖĞµÄ×Ö¶ÎÖ»¼ÇÂ¼ÔÚÈÕÖ¾ÖĞĞÄµÄÊı¾İ¿âÖĞ,ÓÃÀ´ÔÚÈÕÖ¾ÖĞĞÄµÄ
    Êı¾İ¿âÖĞ°´×Ö¶Î½øĞĞË÷Òı,²»»áÔÚ±¾µØÈÕÖ¾ÎÄ¼şÖĞ½øĞĞ¼ÇÂ¼¡£
           Ã¿ÊÕµ½Ò»¸öÇëÇó°üºó£¬¸ù¾İÊÕ°üsocket½âÎö¶Ô¶ËµÄIPµØÖ·£¬²¢½âÎö°üÍ·(ÓĞĞ©ÏµÍ³¿ÉÄÜĞèÒª½âÎö²¿·Ö
   °üÌå)±ã¿ÉµÃµ½LsPkgParam½á¹¹ÖĞµÄ¸÷¸ö×Ö¶Î£¬È»ºóµ÷ÓÃLsClientBegin()º¯Êı¡£Ä³Ğ©ÏµÍ³Ã»ÓĞuinµÄ
   ¸ÅÄî,´«ÈëµÄddwUinÌî³É0¼´¿É


   ²ÎÊıËµÃ÷:
pstLsPkgParam:Ö¸ÏòÒ»¸öLsPkgParam½á¹¹Ìå,¸Ã½á¹¹ÌåµÄ¸÷×Ö¶ÎÒÑÌîĞ´ÍêÕû,²»ÄÜÊÇ¿ÕÖ¸Õë,·ñÔò
º¯ÊıÖ´ĞĞÊ§°Ü


º¯Êı·µ»ØÖµ:
<0: Ê§°Ü
=0:³É¹¦
*/
int LsPreSetParam(LsPkgParam *pstLsPkgParam);




/*
   ¹¦ÄÜËµÃ÷:
   ĞŞ¸ÄLsPkgParam½á¹¹ÖĞµÄdwErrId, dwUserDef1-4, LsPkgParam½á¹¹ÖĞµÄdwErrId, dwUserDef1-4±È½ÏÌØÊâ£¬
   Ò»´Î´¦Àí¹ı³ÌÖĞ£¬¿ÉÄÜĞèÒª¶à´ÎĞŞ¸ÄdwErrId, dwUserDef1-4ÖĞµÄÄ³Ò»¸ö»òÄ³¼¸¸ö×Ö¶Î,¿Éµ÷ÓÃ
   LsSetParam£¨£©½øĞĞĞŞ¸Ä

   ²ÎÊıËµÃ÷:
dwType: È¡Öµ·¶Î§ÎªÏÂÁĞµÄºê¶¨Òå
#define	_LC_ERR_RET_ID_		1  //ĞŞ¸ÄLsPkgParamµÄdwErrId
#define	_LC_USER_DEF1_		2  //ĞŞ¸ÄLsPkgParamµÄdwUserDef1
#define	_LC_USER_DEF2_		3  //ĞŞ¸ÄLsPkgParamµÄdwUserDef2
#define	_LC_USER_DEF3_		4  //ĞŞ¸ÄLsPkgParamµÄdwUserDef3
#define	_LC_USER_DEF4_		5  //ĞŞ¸ÄLsPkgParamµÄdwUserDef4
dwVaule: ÒªÉèÖÃµÄÖµ

º¯Êı·µ»ØÖµ:
=0:³É¹¦,¸Ãº¯ÊıÒ»¶¨»áÖ´ĞĞ³É¹¦
*/
int LsSetParam(uint32_t dwType, uint32_t dwVaule);



/*
   ¹¦ÄÜËµÃ÷:
   µ÷ÓÃLsCheckWriteWL()ÅĞ¶Ï·ñÒª¼ÇÂ¼ÈÕÖ¾

   Ò»°ãLsCheckWriteWL()ºÍLsWaterLog()²»µ¥¶ÀÊ¹ÓÃ,ÎªÁËÊ¹ÓÃ·½±ã£¬¶¨ÒåÁËLOGSYS_WATERºê,¼ÇÂ¼ÈÕÖ¾Ê±ÇëÖ±½ÓÊ¹ÓÃLOGSYS_WATER()ºê

   ²ÎÊıËµÃ÷:
iLogLevel: Òª¼ÇÂ¼ÈÕÖ¾µÄ¼¶±ğ (È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")

º¯Êı·µ»ØÖµ:
=0
*/
int LsCheckWriteWL(int iLogLevel);





/*
   ¹¦ÄÜËµÃ÷:
   µ÷ÓÃLsWaterLog()½øĞĞÈÕÖ¾¼ÇÂ¼»ò·¢ËÍ


   Ò»°ãLsCheckWriteWL()ºÍLsWaterLog()²»µ¥¶ÀÊ¹ÓÃ,ÎªÁËÊ¹ÓÃ·½±ã£¬¶¨ÒåÁËLOGSYS_WATERºê,¼ÇÂ¼ÈÕÖ¾Ê±ÇëÖ±½ÓÊ¹ÓÃLOGSYS_WATER()ºê

   ²ÎÊıËµÃ÷:
szFile:Ô´ÎÄ¼şÃû
iLine:Ô´´úÂëµÄËùÔÚµÄĞĞ
szFunction:Ô´´úÂëµÄËùÔÚµÄº¯Êı
iLogLevel: Òª¼ÇÂ¼ÈÕÖ¾µÄ¼¶±ğ (È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")
sFormat:ÈÕÖ¾µÄ¸ñÊ½
º¯Êı·µ»ØÖµ:
=0
*/
int LsWaterLog(const char * szFile, int iLine, const char * szFunction, int iLogLevel,  const char *sFormat, ...)
__attribute__ ((format(printf,5,6)));

/*
   ¹¦ÄÜËµÃ÷:
   µ÷ÓÃLsWaterLogStr()½øĞĞÈÕÖ¾¼ÇÂ¼»ò·¢ËÍ, ÓÃÓÚLOGSYS_WATER_STR()ºê

   ²ÎÊıËµÃ÷:
szFile:Ô´ÎÄ¼şÃû
iLine:Ô´´úÂëµÄËùÔÚµÄĞĞ
szFunction:Ô´´úÂëµÄËùÔÚµÄº¯Êı
iLogLevel: Òª¼ÇÂ¼ÈÕÖ¾µÄ¼¶±ğ (È¡Öµ·¶Î§0-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")
szLogStr:ÈÕÖ¾×Ö·û´®
º¯Êı·µ»ØÖµ:
=0
*/
int LsWaterLogStr(const char * szFile, int iLine, const char * szFunction, int iLogLevel, char *szLogStr);

/*
   ¹¦ÄÜËµÃ÷:
   ×ª·¢°üº¯Êı£¬¸ù¾İ×ª·¢°üµÄÈ¾É«ÅäÖÃ½«Ä³Ğ©uin»òÃüÁîºÅÆ¥ÅäµÄÇëÇó°ü×ª·¢µ½Ö¸¶¨µÄip:port

   ²ÎÊıËµÃ÷:
iLogLevel: Òª×ª·¢°üµÄ¼¶±ğ (È¡Öµ·¶Î§1-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")
sPkg:Òª×ª·¢µÄÇëÇó°ü,³¤¶ÈÓÉiPkgLenÖ¸¶¨,²»ÄÜÊÇ¿ÕÖ¸Õë
iPkgLen:sPkgµÄ³¤¶È,È¡Öµ·¶Î§(0-64KB)


º¯Êı·µ»ØÖµ:
=0:²»·ûºÏ×ª·¢Ìõ¼ş, Î´×ª·¢
=1:·ûºÏ×ª·¢Ìõ¼ş, ÒÑ×ª·¢
=-10:²ÎÊı´«Èë´íÎó
<0µÄÆäËûÖµ:ÄÚ²¿ÏµÍ³´íÎó
*/
int LsColorRelayPkg(int iLogLevel, void *sPkg, int iPkgLen);

/*
   ¹¦ÄÜËµÃ÷:
   ×ª·¢°üº¯Êı£¬¸ù¾İ×ª·¢°üµÄÈ¾É«ÅäÖÃ½«Ä³Ğ©uin»òÃüÁîºÅÆ¥ÅäµÄÇëÇó°ü×ª·¢µ½Ö¸¶¨µÄip:port

   ²ÎÊıËµÃ÷:
   	iSock: ÓÃÓÚ×ª·¢°üµÄsocket
	iLogLevel: Òª×ª·¢°üµÄ¼¶±ğ (È¡Öµ·¶Î§1-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")
	sPkg:Òª×ª·¢µÄÇëÇó°ü,³¤¶ÈÓÉiPkgLenÖ¸¶¨,²»ÄÜÊÇ¿ÕÖ¸Õë
	iPkgLen:sPkgµÄ³¤¶È,È¡Öµ·¶Î§(0-64KB)


º¯Êı·µ»ØÖµ:
	=0:²»·ûºÏ×ª·¢Ìõ¼ş, Î´×ª·¢
	=1:·ûºÏ×ª·¢Ìõ¼ş, ÒÑ×ª·¢
	=-1:iSockĞ¡ÓÚ0
	=-10:²ÎÊı´«Èë´íÎó
	<0µÄÆäËûÖµ:ÄÚ²¿ÏµÍ³´íÎó
*/
int LsColorRelayPkgWithSock(int iSock, int iLogLevel,  void *sPkg, int iPkgLen);

/*ÉèÖÃ´íÎóĞÅÏ¢*/
int LsSetRetErrMsg(char * szErrMsg);

/*Çå³ı´íÎóĞÅÏ¢*/
int LsClearRetErrMsg();

/*¼ÇÂ¼Í³¼ÆÈÕÖ¾*/
int LsStaticsLog(const char *sFormat, ...);

/*ÊÇ·ñÊÇ¹«Ë¾Ïà¹ØºÅÂë*/
/*
º¯Êı·µ»ØÖµ£º
	=1£ºÊÇ¹«Ë¾Ïà¹ØºÅÂë
	=0£º²»ÊÇ¹«Ë¾Ïà¹ØºÅÂë
	<0£º´íÎó
*/
int LsIsCompanyUin(uint64_t ddwUin);


/*
   APIÊ¹ÓÃËµÃ÷
   1.ÔÚ½ø³Ì³õÊ¼»¯Ê±µ÷ÓÃLsClientInit()
   2.Ã¿ÊÕµ½Ò»¸öÇëÇó°ü»ò¿ªÊ¼Ò»¸öĞÂµÄ´¦ÀíÁ÷³Ì,ĞèÒªµ÷ÓÃLsPreSetParam()
   3.Ê¹ÓÃLOGSYS_WATERºê¼ÇÂ¼ÈÕÖ¾
   4.ÔÚ½ø³ÌÔËÓª¹ı³ÌÖĞ,Èç¹ûÒªĞŞ¸ÄÄ³Ğ©ÅäÖÃÏî,¿Éµ÷ÓÃLsReloadLogCfg()
   5.²»½¨ÒéÖ±½ÓĞŞ¸Äg_stLsClientµÄ³ÉÔ±±äÁ¿
   6.Èç¹ûÒª½«È¾É«ÅäÖÃÃüÖĞµÄÇëÇó°ü×ª·¢µ½Ö¸¶¨µÄip:port,µ÷ÓÃLsColorRelayPkg()º¯Êı
   ²¢ÅĞ¶Ï·µ»ØÖµÊÇ·ñÎª1,Èç¹û·µ»ØÖµÎª1,±íÃ÷×ª·¢³É¹¦,·ñÔò×ª·¢Ê§°Ü
   7.¸ü¶àĞÅÏ¢Çë²Î¿¼¡¶Í³Ò»ÈÕÖ¾ÏµÍ³Ê¹ÓÃËµÃ÷.doc¡·
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


/********************************Ìí¼Ó¶Ô¶àÒµÎñÖ§³ÖºóµÄĞÂµÄ¼ÇÂ¼ÈÕÖ¾µÄº¯Êı*********************************/
/*
!*±£´æÒµÎñµÄÈÕÖ¾¼¶±ğ¼°ÊÇ·ñÊ¹ÓÃÔ¶³ÌÈÕÖ¾µÄĞÅÏ¢£¬
!*Ö÷ÒªÓÃÓÚÒµÎñÔÚÔËĞĞÆÚ¼ä×Ô¼ºĞŞ¸ÄÅäÖÃ
!*/
typedef struct
{
	int8_t cWLogLevel;    /*±¾µØÁ÷Ë®ÈÕÖ¾¼¶±ğ*/
	int8_t cWLogMsgLevel; /*Ô¶³ÌÁ÷Ë®ÈÕÖ¾¼¶±ğ*/
	int8_t cTLogLevel;
	int8_t cTLogMsgLevel;
	int8_t cRelayLogLevel;
}LsLogLevelConf;
	


/************************************************************************************************************/
/*                                          ÉèÖÃ¿Í»§³õÊ¼»¯²ÎÊıµÄÏà¹Ø²Ù×÷                                    */
/************************************************************************************************************/
/*
!*brief : ÓÃ»§×Ô¶¨Òå×Ö¶Î£¬ÓÃÓÚ³õÊ¼»¯Ê±´«µİ¸øLsLogFsOpenº¯Êı£¬Ö÷Òª°üº¬ÁËÊÇ·ñ¿ªÆôÔ¶³ÌÈÕÖ¾£¬Ô¶³ÌÈÕÖ¾ÏµÍ³idµÈĞÅÏ¢¡£
!*/
typedef struct 
{
	int   iSvrConfiged;
	int   iColorSvrConfiged;
	int   m_iWaterDestID;
	int   m_iColorDestID;
    uint32_t dwUserDef1;       //³õÊ¼»¯ÓÃ»§×Ô¶¨Òå×Ö¶Î1£¬ÓÉµ÷ÓÃÕß×Ô¼ºÌîĞ´£¬×Ô¼º½âÊÍ
    uint32_t dwProcId;
	uint16_t wMsgPort;			//¿ÉÖ¸¶¨·¢ËÍÔ¶³ÌÈÕÖ¾Ëù°ó¶¨µÄ¶Ë¿Ú£¬Îª0ÔòËæ»ú¶Ë¿Ú
	uint16_t wRelayPort;		//¿ÉÖ¸¶¨×ª·¢°üËù°ó¶¨µÄ¶Ë¿Ú£¬Îª0ÔòËæ»ú¶Ë¿Ú
	uint16_t cAgentLocalLog;    //ÊÇ·ñÓÉAgentÀ´¸ºÔğ¼ÇÂ¼±¾µØlog£¬ÒµÎñ½ø³Ì²»ÔÙ¼ÇÂ¼
}LOGSYS_CLIENT_INFO;


/*
!*brief : ³õÊ¼»¯ÒµÎñÏà¹ØµÄĞÅÏ¢
!*Ä¬ÈÏ¿ªÆôÔ¶³ÌÁ÷Ë®ÈÕÖ¾ºÍÔ¶³ÌÈ¾É«ÈÕÖ¾(Èç¹ûÅäÖÃÖĞĞÄÓĞ¸ÃÒµÎñµÄÈÕÖ¾¼¶±ğÏà¹ØµÄÅäÖÃ£¬ÔòÊ¹ÓÃÅäÖÃÖĞĞÄµÄÈÕÖ¾¼¶±ğ£¬ÎŞÔòÊ¹ÓÃÄ¬ÈÏ
!*µÄÈÕÖ¾¼¶±ğ:Ô¶³ÌÈÕÖ¾Ê¹ÓÃLC_WARNING£¬±¾µØÁ÷Ë®¼¶±ğÎªLC_WARNING£¬¹Ø±Õ±¾µØÈ¾É«¹¦ÄÜ
!*Ä¬ÈÏ¿ªÆôÓÉagent´úĞ´±¾µØÈÕÖ¾µÄ¹¦ÄÜ
!*
!*×¢Òâ: Èç¹ûËùÓĞµÄÖµ¶¼Ê¹ÓÃÄ¬ÈÏÖµ£¬Ô¶³ÌÈÕÖ¾ÖĞĞÄ»áÊ¹ÓÃÅäÖÃÖĞĞÄµÄÅäÖÃ£¬Òò´ËĞèÒª°²×°×îĞÂµÄagent£¬·ñÔò
!*ĞèÒªµ÷ÓÃLsLogFsClientInfoSetLogDestIDº¯ÊıÉèÖÃÔ¶³ÌÈÕÖ¾ÖĞĞÄµÄid¡£
!*/

int LsLogFsClientInfoInit(LOGSYS_CLIENT_INFO *pstClientInfo);

/*
!*ÉèÖÃ½ø³ÌºÅ,ÔÚÓÉapi×Ô¼ºĞ´±¾µØÈÕÖ¾Ê±¿ÉÓÃÀ´Çø·Ö²»Í¬µÄ½ø³Ì
!*/
int LsLogFsClientInfoSetProcId(LOGSYS_CLIENT_INFO *pstClientInfo, uint32_t value);

/*
!*ÉèÖÃ³õÊ¼»¯Ê±µÄuserdefÏî¡£
!*/
int LsLogFsClientInfoSetInitUserDef(LOGSYS_CLIENT_INFO *pstClientInfo, uint32_t value);

/*
!*ÉèÖÃÊÇ·ñÊ¹ÓÃÔ¶³ÌÈ¾É«ÈÕÖ¾ÖĞĞÄ£¬value=0±íÊ¾·ñ£¬ÆäÓà±íÊ¾ÊÇ
!*/
int LsLogFsClientInfoSetUseColorSvr(LOGSYS_CLIENT_INFO *pstClientInfo, int value);

/*
!*ÉèÖÃÊÇ·ñ¼ÇÂ¼Ô¶³ÌÁ÷Ë®ÈÕÖ¾£¬value=0±íÊ¾·ñ£¬ÆäÓà±íÊ¾ÊÇ
!*/
int LsLogFsClientInfoSetUseWaterSvr(LOGSYS_CLIENT_INFO *pstClientInfo, int value);

/*
!*ÉèÖÃÊı¾İ°ü×ª·¢Ê±ËùÊ¹ÓÃµÄ¶Ë¿ÚºÅ
!*/
int LsLogFsClientInfoSetRelayPort(LOGSYS_CLIENT_INFO *pstClientInfo, uint16_t value);

/*ÉèÖÃÔ¶³ÌÈ¾É«ÅäÖÃµÄÏµÍ³idºÍÔ¶³ÌÁ÷Ë®ÈÕÖ¾µÄÏµÍ³id*/
int LsLogFsClientInfoSetLogDestID(LOGSYS_CLIENT_INFO *pstClientInfo, int iWaterDestID, int iColorDestID);




/*
!*ÓÉÒµÎñ·½³õÊ¼»¯Ê±µ÷ÓÃ£¬²»ĞèÒªĞ¯´øÈÎºÎÅäÖÃ
!*brief :Ö»Ê¹ÓÃÒµÎñÃû»òÕßID½øĞĞ³õÊ¼»¯µÄº¯Êı£¬Ä¬ÈÏ¿ªÆôÔ¶³ÌÈÕÖ¾¹¦ÄÜ£¬¹Ø±Õ±¾µØÈÕÖ¾¹¦ÄÜ
!*param[in]:szAppName ,ÒµÎñÃû³Æ£¬ÔÚdwAppId²»Îª0Ê±¿ÉÒÔÎªNULL£¬×î³¤255
!*param[in]:szUserName,ÒµÎñ¸ºÔğÈËÃû³Æ£¬ÔÚdwAppId²»Îª0Ê±¿ÉÒÔÎªNULL£¬×î³¤Îª1024
!*param[in]:dwAppId   ,ÒµÎñID£¬Èç²»ÖªÔòÌî0
!*retval >=0 :Ò»¸öÕıÈ·µÄ¾ä±ú£¬other³õÊ¼»¯´íÎó
!*±¾µØÈÕÖ¾Ïà¹ØµÄĞÅÏ¢£¬°üº¬ÁË±¾µØÈÕÖ¾µÄÎÄ¼şÊı£¬Ã¿¸öÈÕÖ¾ÎÄ¼şµÄ´óĞ¡¼°ÈÕÖ¾ÎÄ¼şµÄ¾ø¶ÔÂ·¾¶
!*/
	int LsLogFsOpenNoConf(const char * szAppName, const char * szUserName, const uint32_t dwAppId,
								LsLogFileInfo *pstLsLogFileInfo);


/*
!*ÓÉÒµÎñ·½µ÷ÓÃ£¬ÔÚĞ´ÈÕÖ¾Ö®Ç°ĞèÒªµ÷ÓÃ¸Ãº¯Êı
!*brief    :¸ù¾İÒµÎñÃû³Æ£¬»ñÈ¡Ò»¸ö¶ÔÓ¦µÄ¾ä±ú 
!*param[in]:szAppName		,ÒµÎñÃû³Æ£¬ÔÚdwAppId²»Îª0Ê±¿ÉÒÔÎªNULL£¬×î³¤255
!*param[in]:szUserName		 ,ÒµÎñ¸ºÔğÈËÃû³Æ£¬ÔÚdwAppId²»Îª0Ê±¿ÉÒÔÎªNULL£¬×î³¤Îª1024
!*param[in]:iSvrConfiged	  ,ÊÇ·ñÊ¹ÓÃÔ¶³ÌÁ÷Ë®ÈÕÖ¾ÖĞĞÄ
!*param[in]:iColorSvrConfiged ,ÊÇ·ñÊ¹ÓÃÔ¶³ÌÈ¾É«ÈÕÖ¾ÖĞĞÄ
!*param[in]:pstLsInitParam	  ,Ö¸ÏòLsInitParam½á¹¹Ìå,ÓÃÓÚ´«Èë½ø³Ì³õÊ¼»¯ÊÇ¿ÉÒÔÈ·¶¨µÄĞÅÏ¢:±¾»úAppid,³õÊ¼»¯ÓÃ»§×Ô¶¨Òå×Ö¶Î1,½ø³ÌID,²»ÄÜ´«Èë¿ÕÖ¸Õë,·ñÔòº¯Êıµ÷ÓÃ³ö´í
!*param[in]:pstLsLogFileInfo  ,ÓÃÓÚ´«ÈëÒµÎñµÄ±¾µØÈÕÖ¾Ïà¹ØµÄÅäÖÃ£¬±ÈÈç±¾µØÈÕÖ¾µÄÎÄ¼şÃû£¬ÎÄ¼ş´óĞ¡¼°ÎÄ¼ş¸öÊı
!*retval >=0 :Ò»¸öÕıÈ·µÄ¾ä±ú£¬other³õÊ¼»¯´íÎó
!*/
/*
!*µÈ½øÒ»²½È·ÈÏÁË×Ô¶¯×¢²áÁ÷³ÌÔÙÀ´È·ÈÏ¸Ãº¯Êı
!*/
int LsLogFsOpen(const LOGSYS_CLIENT_INFO *pstClientInfo,
                         const char * szAppName, const char * szUserName,  const uint32_t dwAppId,
                         LsLogFileInfo *pstLsLogFileInfo);

/*
!*
!*brief : ¹Ø±ÕÒ»¸ö¾ä±ú
!*param[in] iFd : Òª¹Ø±ÕµÄlog fileÃèÊö·û
!*retval 0±íÊ¾ÕıÈ·£¬other³õÊ¼»¯´íÎó
!*/
int LsLogFsClose(const int iFd);

/*
!*
!*brief : ÏòÒ»¸ö¸ø¶¨¾ä±úÖĞĞ´ÈëÈÕÖ¾ĞÅÏ¢£¬Ê×ÏÈ»ñÈ¡ÓëFD¶ÔÓ¦µÄÊµÀı£¬È»ºóÊ¹ÓÃÆäÖĞ±£´æµÄÅäÖÃĞÅÏ¢À´ÎªÒµÎñĞ´ÈÕÖ¾ĞÅÏ¢
!*/
int LsLogFsWrite(const int iFd, const char * szFile, int iLine, 
					 const char * szFunction, int iLogLevel, const char *sFormat, ...)
__attribute__ ((format(printf,6,7)));

/*
!*brief : ÒµÎñ·½Ìá¹©va_list×÷Îª²ÎÊıµÄĞ´ÈÕÖ¾º¯Êı
!*param[in] iFd : ³õÊ¼»¯Ê±·µ»ØµÄÎÄ¼şÃèÊö·û
!*param[in] va	: ÒªĞ´ÈëµÄvalist²ÎÊı
!*/
int LsLogFsWriteVA(const int iFd, const char * szFile, int iLine, 
						const char * szFunction, int iLogLevel, const char *sFormat, va_list va);


/*
!*brief : ×ª·¢Êı¾İ°üµÄº¯Êı
!*param[in] iFd : ÓÉ³õÊ¼»¯º¯Êı·µ»ØµÄÓëÒµÎñ¶ÔÓ¦µÄ¾ä±ú
!*param[in] iLogLevel : ¼¶±ğ
!*param[in] sPkg : ĞèÒª×ª·¢µÄÊı¾İ°ü
!*param[in] iPkgLen : ĞèÒª×ª·¢µÄÊı¾İ°üµÄ³¤¶È
!*retval : 1±íÊ¾³É¹¦£¬ other ±íÊ¾Ê§°Ü
!*/
int LsLogFsRelayPkg(const int iFd, const int iLogLevel,  const void *sPkg, const int iPkgLen);

/*
!*brief : ÉèÖÃµ±Ç°ÒµÎñµÄÈÕÖ¾ĞÅÏ¢£¬°üÀ¨userdef£¬uinµÈµÈ
!*param[in] : iFd ÒªÉèÖÃµÄÒµÎñËù¶ÔÓ¦µÄÎÄ¼şÃèÊö·û
!*param[in] : pstLsPktParam ÒªÉèÖÃµÄÊı¾İ°üµÄ²ÎÊı£¬¸ÃÖ¸Õë²»ÄÜÎª¿Õ
!*retval	: 0 ³É¹¦£¬ other Ê§°Ü 
!*/
int LsLogFsPreSetPktParam(int iFd, LsPkgParam *pstLsPktParam);

/*
!*brief : ¸ù¾İÀàĞÍÉèÖÃÊı¾İ°ü²ÎÊı£¬ÕâĞ©²ÎÊıÓÃÓÚÔÚÆ¥ÅäÈ¾É«ÅäÖÃÊ±Ê¹ÓÃ
!*param[in] : iFd ,openº¯Êı·µ»Ø¸øÒµÎñµÄÃèÊö·û
!*param[in] : dwType , ÒªÉèÖÃµÄ²ÎÊıµÄÀàĞÍ
!*param[in] : dwValue, ÒªÉèÖÃµÄ²ÎÊıµÄÖµ
!*retval	: 0 ³É¹¦£¬ other Ê§°Ü 
!*/
int LsLogFsSetPktParam(int iFd, uint32_t dwType, uint32_t dwVaule);

/*
!*brief : »ñÈ¡Ò»¸öÒµÎñµÄÈÕÖ¾¼¶±ğµÈÅäÖÃĞÅÏ¢
!*param[in] : iFd ,³õÊ¼»¯Ê±·µ»Ø¸øÒµÎñ·½µÄÎÄ¼şÃèÊö·û
!*param[out]: pstLsLogLevelConf, ÒµÎñµÄÈÕÖ¾¼¶±ğµÈÅäÖÃ
!*/
int LsLogGetConf(int iFd, LsLogLevelConf *pstLsLogLevelConf);

/*
!*brief : ÉèÖÃÈÕÖ¾¼¶±ğµÈÅäÖÃĞÅÏ¢
!*param[in] : iFd ,³õÊ¼»¯Ê±·µ»Ø¸øÒµÎñ·½µÄÎÄ¼şÃèÊö·û
!*param[in] : pstLsLogLevelConf , ÒªÉèÖÃµÄĞÂµÄÈÕÖ¾¼¶±ğµÈĞÅÏ¢£¬
!retval 	: 0 LOGSYS_EOK ³É¹¦£¬ÆäÓàÖµ±íÊ¾Ê§°Ü
!*ps : Èç¹ûÔÚÉèÖÃ¸Ãº¯Êıºó£¬ÅäÖÃÖĞĞÄµÄÒµÎñÈÕÖ¾¼¶±ğÓĞĞŞ¸ÄÊ±ÒÔÅäÖÃÖĞĞÄµÄÅäÖÃÎª×¼
!*/
int LsLogSetConf(int iFd, LsLogLevelConf *pstLsLogLevelConf);


/*
!*brief :³õ²½ÅĞ¶ÏÒ»ÌõÈÕÖ¾µÄÈÕÖ¾¼¶±ğÊÇ·ñÂú×ã·¢ËÍµÄÌõ¼ş,²»¶ÔÈ¾É«ÅäÖÃ½øĞĞÆ¥Åä
!*param[in] iFd: ÒªÅĞ¶ÏµÄÒµÎñ¶ÔÓ¦µÄÎÄ¼şÃèÊö·û
!*param[in] iLogLevel ÈÕÖ¾¼¶±ğ
!*retval 1 ¸ÃÈÕÖ¾¼¶±ğµÄÈÕÖ¾ĞèÒª±»½øÒ»²½´¦Àí£¬0 ¸Ã¼¶±ğµÄÈÕÖ¾²»ĞèÒª±»½øÒ»²½´¦Àí£¬other ´íÎó
!*/
int LsLogFsCheckLevel(const int iFd, const int iLogLevel);


/*
   APIÊ¹ÓÃËµÃ÷
   1.ÔÚ½ø³Ì³õÊ¼»¯Ê±µ÷ÓÃLsLogFsOpen »òÕßLsLogFsOpenNoCfgº¯Êı»ñÈ¡Ò»¸ö¾ä±ú
   2.Ã¿ÊÕµ½Ò»¸öÇëÇó°ü»ò¿ªÊ¼Ò»¸öĞÂµÄ´¦ÀíÁ÷³Ì,ĞèÒªµ÷ÓÃLsLogFsPreSetPktParamÀ´ÉèÖÃÏà¹ØµÄ²ÎÊı
   3.Ê¹ÓÃLOGSYS_WATER_V2ºê¼ÇÂ¼ÈÕÖ¾
   4.ÔÚ½ø³ÌÔËÓª¹ı³ÌÖĞ,Èç¹ûÒªĞŞ¸ÄÄ³Ğ©ÅäÖÃÏî,¿Éµ÷ÓÃLsLogSetConf()À´ÉèÖÃĞÂµÄÅäÖÃ£¬Ò²¿ÉÒÔÊ¹ÓÃLsLogGetConfÀ´»ñÈ¡¾ÉµÄÅäÖÃ
   5.²»½¨ÒéÖ±½ÓĞŞ¸Äg_stLsClientµÄ³ÉÔ±±äÁ¿
   6.Èç¹ûÒª½«È¾É«ÅäÖÃÃüÖĞµÄÇëÇó°ü×ª·¢µ½Ö¸¶¨µÄip:port,µ÷ÓÃLsLogFsRelayPkg()º¯Êı
   ²¢ÅĞ¶Ï·µ»ØÖµÊÇ·ñÎª1,Èç¹û·µ»ØÖµÎª1,±íÃ÷×ª·¢³É¹¦,·ñÔò×ª·¢Ê§°Ü
   7.¸ü¶àĞÅÏ¢Çë²Î¿¼¡¶Í³Ò»ÈÕÖ¾ÏµÍ³Ê¹ÓÃËµÃ÷.doc¡·
*/

#define LOGSYS_WATER_V2(iFd, iLogLevel, fmt, args...) do{\
	if (LsLogFsCheckLevel(iFd, iLogLevel)) \
	LsLogFsWrite(iFd, __FILE__, __LINE__, __FUNCTION__, iLogLevel, fmt, ##args);\
}while(0)

/*ÒÔvalistÎª²ÎÊı*/
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
   ¹¦ÄÜËµÃ÷:
   ×ª·¢°üº¯Êı£¬¸ù¾İ×ª·¢°üµÄÈ¾É«ÅäÖÃ½«Ä³Ğ©uin»òÃüÁîºÅÆ¥ÅäµÄÇëÇó°ü×ª·¢µ½Ö¸¶¨µÄip:port

   ²ÎÊıËµÃ÷:
iLogLevel: Òª×ª·¢°üµÄ¼¶±ğ (È¡Öµ·¶Î§1-5,Çë²Î¿¼"ÈÕÖ¾¼¶±ğ¶¨Òå")
sPkg:Òª×ª·¢µÄÇëÇó°ü,³¤¶ÈÓÉiPkgLenÖ¸¶¨,²»ÄÜÊÇ¿ÕÖ¸Õë
iPkgLen:sPkgµÄ³¤¶È,È¡Öµ·¶Î§(0-64KB)


º¯Êı·µ»ØÖµ:
=0:²»·ûºÏ×ª·¢Ìõ¼ş, Î´×ª·¢
=1:·ûºÏ×ª·¢Ìõ¼ş, ÒÑ×ª·¢
=-10:²ÎÊı´«Èë´íÎó
<0µÄÆäËûÖµ:ÄÚ²¿ÏµÍ³´íÎó
*/

/*********************************************end*******************************************************/
}

#endif

