/**
 *  * spp exception signal handle
 *   *
 *    **/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "exception.h"
#include "serverbase.h"

namespace spp
{
namespace exception
{

#define EXCEP_LOGPATITION     "/data/"
#define EXCEP_LOG_FILEPATH    "./spp_exception_log"
#define EXCEP_DUMPPACKLEN     (64*1024)
#define IPV4_STRLEN            16

bool _spp_g_exceptionpacketdump = false;
bool _spp_g_exceptioncoredump   = true;
bool _spp_g_exceptionrestart   = true;
bool _spp_g_exceptionmonitor   = true;
bool _spp_g_exceptionrealtime = true;

CServerBase *_spp_g_base = NULL;

GET_MSG_FUN _spp_g_get_msg_fun_ptr = NULL;
CHECK_STACK_FUN _spp_g_check_stack_fun_ptr = NULL;

struct PacketInfo
{
    int64_t sustime_;
    int     srcip_;
    int     dstip_;
    int     srcport_;
    int     dstport_;
    int     bufflen_;
    int     packlen_;
    void   *packetbuff_;
    PacketInfo() 
    {
        sustime_ = 0;
        srcip_   = 0;
        dstip_   = 0;
        srcport_ = 0;
        dstport_ = 0;
        bufflen_ = 0;
        packlen_ = 0;
        packetbuff_ = NULL;
    }
};

struct ExceptionInfo
{
    int        signo_;   //unused
    PacketInfo packinfo_[PACK_TYPE_BUTT];
    ExceptionInfo(): signo_(0){}
};

ExceptionInfo g_stExceptionInfo;

static const char *DumpPacket(void *pPkg, int iPkgLen)
{
	int i, inc;
	static char sLogBuf[65536*4];
	char *pDst = (char *)sLogBuf, *pSrc = (char *)pPkg;

	sLogBuf[0] = '\0';
	for (i = 0; i< iPkgLen; i++, pSrc++) {
		const char *sSep;
		char sAddr[20];
		switch (i % 16) {
			case 3:
			case 11: sSep = " - "; break;
			case 7:  sSep = " | "; break;
			case 15: sSep = "\n"; break;
			default: sSep = " "; break;
		}
		switch (i % 16) {
			case 0: snprintf(sAddr, sizeof(sAddr), "%04hX: ", i); break;
			default: sAddr[0] = '\0'; break;
		}
		inc = snprintf(pDst, sLogBuf + sizeof(sLogBuf) - pDst, "%s%02X%s", sAddr, (unsigned char) *pSrc, sSep);
		if (inc < 0) { break; }
		pDst += inc;
		if (pDst >= sLogBuf + sizeof(sLogBuf)) { break; }
	}

	return sLogBuf;
}

static char *Time2Str(int64_t timesec, int type)
{
    static char szTimeStr[32] = {0};
    struct tm   stTimeVal;
    time_t      lTime = timesec;

    localtime_r(&lTime, &stTimeVal);

    /* for file name */
    if (1 == type)
    {
        snprintf(szTimeStr, sizeof(szTimeStr) - 1, "%02d%02d%02d%02d%02d", 
                 stTimeVal.tm_mon+1, stTimeVal.tm_mday, stTimeVal.tm_hour, 
                 stTimeVal.tm_min, stTimeVal.tm_sec);
        return szTimeStr;
    }

    /* for log time print */
    if (stTimeVal.tm_year > 50)
    {
        snprintf(szTimeStr, sizeof(szTimeStr)-1, "%04d-%02d-%02d %02d:%02d:%02d", 
                 stTimeVal.tm_year+1900, stTimeVal.tm_mon+1, stTimeVal.tm_mday,
                 stTimeVal.tm_hour, stTimeVal.tm_min, stTimeVal.tm_sec);
    }
    else
    {
        snprintf(szTimeStr, sizeof(szTimeStr)-1, "%04d-%02d-%02d %02d:%02d:%02d", 
                 stTimeVal.tm_year+2000, stTimeVal.tm_mon+1, stTimeVal.tm_mday,
                 stTimeVal.tm_hour, stTimeVal.tm_min, stTimeVal.tm_sec);
    }

    return szTimeStr;
}

char *TruncateInsName(char *cmdline)
{
    char *pos;
    char *begin;

    pos = strstr(cmdline, "spp_");
    if (NULL == pos)
    {
        return NULL;
    }

    begin = pos + 4;

    pos = strstr(cmdline, "_worker");
    if (NULL == pos)
    {
        pos = strstr(cmdline, "_proxy");
        if (NULL == pos)
        {
            return NULL;
        }
    }

    if (begin >= pos)
    {
        return NULL;
    }

    *pos = '\0';

    return begin;
}

char* GetCmdline(char *cmdline, int len)
{
    #define FILE_PATH_MAX 128

    int fd;
    int readlen;
    pid_t pid = getpid();
    char path[FILE_PATH_MAX] = {0};

    snprintf(path, FILE_PATH_MAX, "/proc/%d/cmdline", pid);

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    readlen = read(fd, cmdline, len - 1);
    if (readlen < 0)
    {
        close(fd);
        return NULL;
    }

    cmdline[readlen] = '\0';

    close(fd);
    return cmdline;
}


char *GetSppInstanceName(char *ins_name, int len)
{
    #define CMD_LINE_LEN  128

    char* name = NULL;
    char  cmdline[CMD_LINE_LEN] = {0};

    if (NULL == GetCmdline(cmdline, CMD_LINE_LEN))
    {
        return NULL;
    }

    name = TruncateInsName(cmdline);
    if (!name)
    {
        return NULL;
    }

    snprintf(ins_name, len - 1, "%s", name);

    return ins_name;
}


char *GetFileName(int iFileType) 
{
    #define SPP_INSTANCE_NAME_LEN 128

    time_t iCurTime;
    static char filename[64] = {0};
    char  name[SPP_INSTANCE_NAME_LEN] = {0};
    char* pos = GetSppInstanceName(name, SPP_INSTANCE_NAME_LEN);

    iCurTime = time(NULL);

    if (1 == iFileType)
    {//gstack filename
        snprintf(filename, sizeof(filename) - 1, "%s/%s/spp_gstack.%s.%s", EXCEP_LOGPATITION, EXCEP_LOG_FILEPATH, name, Time2Str(iCurTime, 1));
    }
    else if (2 == iFileType)
    {//exception log filename
        if (pos)
        {
            snprintf(filename, sizeof(filename) - 1, "%s/%s/deal_exception.%s", EXCEP_LOGPATITION, EXCEP_LOG_FILEPATH, name);
        }
        else
        {
            snprintf(filename, sizeof(filename) - 1, "%s/%s/deal_exception", EXCEP_LOGPATITION, EXCEP_LOG_FILEPATH);
        }
    }
    else
    {//packet dump filename
        snprintf(filename, sizeof(filename) - 1, "%s/%s/spp_pack.%s.%d", EXCEP_LOGPATITION, EXCEP_LOG_FILEPATH, Time2Str(iCurTime, 1), getpid());
    }

    return filename;
}

void InitHanleException(void)
{
    char dir[64] = {'\0'};
    snprintf(dir, sizeof(dir) - 1, "%s/%s", EXCEP_LOGPATITION, EXCEP_LOG_FILEPATH);
    mkdir(dir, 0775);

    return;
}

void GstackLog(int pid)
{
    char cmd_buf[128] = {0};

    snprintf(cmd_buf, sizeof(cmd_buf) - 1, "gstack %d > %s", pid, GetFileName(1)); 
    system(cmd_buf);
    return;
}

static inline void Inetip2asc(int ip, char *szip, int len)
{
    char *ipStr = NULL;
    ipStr = inet_ntoa(*(in_addr *)&ip);
    snprintf(szip, len, "%s", ipStr);

    return;
}

void DumpPackageToFile(long lESP)
{
    int   fd;
    int   i;
    char  szSrcIp[IPV4_STRLEN];
    char  szDstIp[IPV4_STRLEN];
    char  szSeparator[128] = "\n-------------------------------------------------------------------\n";

    char *pFileName = GetFileName(0);
    
    fd = open(pFileName, O_RDWR | O_CREAT, 0777);
    if (fd < 0)
    {
        fprintf(stderr, "Open packet dump file error, %s.\n", pFileName);
        return;
    }

    for (i = 0; i < PACK_TYPE_BUTT; i++)
    {
        PacketInfo *pstPacketInfo = &(g_stExceptionInfo.packinfo_[i]);
        const char *pack;
        int         len;
        char        buf[128];

        if (0 == pstPacketInfo->packlen_)
        {
            continue;
        }

        len = strlen(szSeparator);
        if (write(fd, szSeparator, len) < len)
        {
            fprintf(stderr, "Write separator error.\n");
            continue;
        }

        Inetip2asc(pstPacketInfo->srcip_, szSrcIp, IPV4_STRLEN);
        Inetip2asc(pstPacketInfo->dstip_, szDstIp, IPV4_STRLEN);
        snprintf(buf, sizeof(buf), "[%s.%llu]  %s[%d] > %s[%d]  type:%d\n",\
                 Time2Str(pstPacketInfo->sustime_/1000, 0), (unsigned long long)pstPacketInfo->sustime_%1000, \
                 szSrcIp, pstPacketInfo->srcport_, szDstIp, pstPacketInfo->dstport_, i);

        len = strlen(buf);
        if (write(fd, buf, len) < len)
        {
            fprintf(stderr, "Write packet info error, %s, %d.\n", pFileName, i);
            continue;
        }

        pack = DumpPacket(pstPacketInfo->packetbuff_, pstPacketInfo->packlen_);
        len  = strlen(pack);
        if (write(fd, pack, len) < len)
        {
            fprintf(stderr, "Write packet dump file error, %s, %d.\n", pFileName, i);
            continue;
        }

        len = strlen(szSeparator);
        if (write(fd, szSeparator, len) < len)
        {
            fprintf(stderr, "Write separator error.\n");
            continue;
        }
    }

    if (_spp_g_get_msg_fun_ptr)
    {
	    string strReq = _spp_g_get_msg_fun_ptr();

	    const char *pMsg = "\n----------MSG----------\n";
	    write(fd, pMsg, strlen(pMsg));

	    pMsg = DumpPacket((void *)strReq.data(), strReq.size());
	    write(fd, pMsg, strlen(pMsg));
    }
	
	if (lESP && _spp_g_check_stack_fun_ptr)
	{
		bool bHealth = _spp_g_check_stack_fun_ptr(lESP);
		if (bHealth)
		{
		    const char *pMsg = "\n----------in of stack----------\n";
		    write(fd, pMsg, strlen(pMsg));
		}
		else
		{
		    const char *pMsg = "\n----------out of stack----------\n";
		    write(fd, pMsg, strlen(pMsg));
		}
	}

    close(fd);

    return;
    
}

void SavePackage(PackType enPacktype, const PacketData *pstPacketData)
{
    if ((NULL == pstPacketData) || (0 == pstPacketData->packlen_))
    {
        return;
    }

    switch (enPacktype)
    {
        case PACK_TYPE_FRONT_RECV:
        case PACK_TYPE_BACK_RECV:
        {
            PacketInfo *pstPacketInfo = &(g_stExceptionInfo.packinfo_[enPacktype]);
            int  copylen;

            if (NULL == pstPacketInfo->packetbuff_)
            {
                pstPacketInfo->packetbuff_ = malloc(EXCEP_DUMPPACKLEN);
                if (NULL == pstPacketInfo->packetbuff_)
                {
                    return;
                }

                pstPacketInfo->bufflen_ = EXCEP_DUMPPACKLEN;
            }

            copylen = ((pstPacketData->packlen_ > pstPacketInfo->bufflen_) ? pstPacketInfo->bufflen_ : pstPacketData->packlen_);
            memcpy(pstPacketInfo->packetbuff_, pstPacketData->packetbuff_, copylen);
            pstPacketInfo->packlen_ = copylen;
            pstPacketInfo->dstip_   = pstPacketData->dstip_;
            pstPacketInfo->srcip_   = pstPacketData->srcip_;
            pstPacketInfo->dstport_ = pstPacketData->dstport_;
            pstPacketInfo->srcport_ = pstPacketData->srcport_;
            pstPacketInfo->sustime_ = pstPacketData->sustime_;

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void set_serverbase(CServerBase *base)
{
    _spp_g_base = base;
}

void deal_workexception(long lESP, int signo)
{
    if (_spp_g_base)
    {
        _spp_g_base->moncli_.exception_report(signo);
    }

    if (_spp_g_exceptionpacketdump)
    {
        DumpPackageToFile(lESP);
    }

    return;
}


}
}



