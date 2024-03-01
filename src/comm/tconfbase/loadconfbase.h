#ifndef _SPP_LOADCONFBASE_H_
#define _SPP_LOADCONFBASE_H_


#include<stdio.h>
#include<stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <string>
#include "singleton.h"
#include "tlog.h"

#define  LOG_CONF_SCREEN(lvl, fmt, args...)   do{ printf("[%d] ", getpid());printf(fmt, ##args); INTERNAL_LOG->LOG_P_PID(lvl, fmt, ##args); }while (0)
using namespace spp::singleton;
using namespace tbase::tlog;
namespace spp
{
namespace tconfbase
{
struct Log
{
    int level;
    int type;
    int maxfilesize;
    int maxfilenum;
    std::string path;
    std::string prefix;

    Log(std::string pre){clear(pre);}
    Log(){clear();}
    void clear(std::string pre="")
    {
        level = LOG_ERROR;
        type = LOG_TYPE_CYCLE;
        path = "../log";
        prefix = pre;
        maxfilesize = 10485760;
        maxfilenum  = 10;
    }
};
struct Stat
{
    std::string file;

    Stat(){clear();}
    void clear(std::string _file="")
    {
        file = _file;
    }
};

struct Limit
{
    unsigned sendcache;

    void clear()
    {
        sendcache = 0;
    }
};
struct ProcmonEntry
{
    int id;
    std::string basepath;
    std::string exe;
    std::string etc;
    unsigned maxprocnum;
    unsigned minprocnum;
    unsigned heartbeat;
    unsigned affinity;
    unsigned reload;
    int exitsignal;

    ProcmonEntry()
    {
        id = 0;
        basepath = ".";
        maxprocnum = 1;
        minprocnum = 1;
        heartbeat = 60;
        exitsignal = 9;
        affinity = 0;
        reload = 0;
    }

};
struct Procmon
{
    std::vector<ProcmonEntry> entry;

    void clear()
    {
        entry.clear();
    }
};
struct Moni
{
    Log log;
    int intervial;
    Moni(std::string prefix):log(prefix), intervial(30){}
    Moni():intervial(30){}
    void clear(std::string pre = "")
    {
        log.clear(pre);
        intervial = 30;
    }


};
struct Module
{
    std::string bin;
    std::string etc;
    bool isGlobal;
    std::string localHandleName;

    Module(){clear();}
    void clear()
    {
        bin = "";
        etc = "";
        isGlobal = 0;
        localHandleName = "";
    }
};

struct Result
{
    std::string bin;

    Result(){clear();}
    void clear()
    {
        bin = "";
    }
};

struct ConnectEntry
{
    int id;
    std::string type;
    int recv_size;
    int send_size;
    unsigned msg_timeout;
    ConnectEntry()
    {
       type = "shm";
       recv_size = 16;
       send_size = 16;
       msg_timeout = 0;
       id = 0;
    }

};
struct ConnectShm
{

    std::vector<ConnectEntry> entry;
    std::string type;
    std::string scriptname;

    ConnectShm(){clear();}
    void clear()
    {
        entry.clear();
        type = "shm";
        scriptname = "";
    }
};
struct AcceptorEntry
{
    std::string type;
    std::string ip;
    unsigned short port;
    std::string path;
    int TOS;
    int oob;
    bool abstract;
    AcceptorEntry():ip("eth1"),  port(0), TOS(-1), oob(0), abstract(1){}

};
struct AcceptorSock
{
    std::vector<AcceptorEntry> entry;
    int timeout;
    bool udpclose;
    int maxconn;
    int maxpkg;
    std::string type;
    AcceptorSock(){clear();}

    void clear()
    {
        entry.clear();
        timeout = 60;
        maxconn = 100000;
        maxpkg = 100000;
        udpclose = 1;
        type = "socket";
    }
};

struct SessionConfig
{
    std::string etc;
    int epollTime;
    SessionConfig(){clear();}
    void clear()
    {
        etc = "";
        epollTime = 0;
    }
};
struct Report
{
    std::string report_tool;
    Report(){clear();}
    void clear()
    {
        report_tool = "";
    }
};
struct Proxy
{
    int groupid;
    int shm_fifo;
    Proxy(){clear();}
    void clear()
    {
        groupid  = 0;
        shm_fifo = 0;
    }
};
struct Worker
{
    int groupid;
    int TOS;
    int L5us;
    int shm_fifo;
    int link_timeout_limit;
    std::string proxy;

    Worker(){clear();}
    void clear()
    {
        groupid = 0;
        TOS = -1;
        L5us = 0;
        shm_fifo = 0;
        link_timeout_limit = 0;     
    }
};
struct Ctrl
{
    void clear()
    {
    }
};
struct Iptable
{
    std::string whitelist;
    std::string blacklist;
    void clear()
    {
        whitelist = "";
        blacklist = "";
    }
};
struct AsyncSessionEntry
{
    int id;
    std::string maintype;
    std::string subtype;
    std::string ip;
    int port;
    int recv_count;
    int timeout;
    int multi_con_inf;
    int multi_con_sup;
    AsyncSessionEntry(){clear();};

    void clear()
    {
        multi_con_inf = 10;
        multi_con_sup = 50;
    }
};
struct AsyncSession
{
    std::vector<AsyncSessionEntry> entry;
    void clear()
    {
        entry.clear();
    }
};

struct Exception
{
    void clear()
    {
        coredump = true;
        packetdump = false;
		restart = true;
		monitor = true;
		realtime = true;
    }
		
    bool coredump;
    bool packetdump;
	bool restart;
	bool monitor;
	bool realtime;
};

struct CGroupConf
{
	uint32_t cpu_sys_reserved_percent;   //  precent 20%
	uint32_t mem_sys_reserved_mb;   // 1024 MB	

	uint32_t cpu_service_percent;  // precent 40%
	uint32_t mem_service_percent;  // 50%

	uint32_t cpu_tools_percent;     // precent 20%
	uint32_t mem_tools_percent;     // 30%

	CGroupConf():cpu_sys_reserved_percent(20),mem_sys_reserved_mb(1024),
					cpu_service_percent(40),mem_service_percent(50),
					cpu_tools_percent(20),mem_tools_percent(50)
	{}

	int32_t check()
	{
	    if(cpu_sys_reserved_percent<1 || cpu_sys_reserved_percent>80
		 || mem_sys_reserved_mb<1 
		 || cpu_service_percent<1 || cpu_service_percent>80
		 || mem_service_percent<1 || mem_service_percent>80
		 || cpu_tools_percent<1 || cpu_tools_percent>80
		 || mem_tools_percent<1 || mem_tools_percent>80)
	    {
	        return -1;
	    }

	    return 0;
	}
};

// agent集群相关配置
struct ClusterConf
{
    std::string type;    // 集群类型: spp or sf2

    ClusterConf() { clear(); }

    void clear()
    {
        type = "spp";
    }
};

enum ELOADCONFBASE
{
    ERR_LOAD_SUCCESS = 0,
    ERR_LOAD_CONF_FILE,
    ERR_NOT_INIT,
    ERR_NO_SUCH_ELEM,

    ERR_END, //表示错误枚举结束无含义

};
class CLoadConfBase
{
    public:
        CLoadConfBase(const std::string& filename);
        virtual ~CLoadConfBase();

        virtual int loadConfFile() = 0;
        virtual void fini() = 0;

        virtual int getAcceptorValue(AcceptorSock& acceptor, std::vector<std::string> vec) = 0;
        virtual int getLogValue(Log& log, std::vector<std::string> vec) = 0;
        virtual int getLimitValue(Limit& limit, std::vector<std::string> vec) = 0;
        virtual int getStatValue(Stat& stat, std::vector<std::string> vec) = 0;
        virtual int getProcmonValue(Procmon& procmon, std::vector<std::string> vec) = 0;
        virtual int getModuleValue(Module& module, std::vector<std::string> vec) = 0;		
        virtual int getResultValue(Result& result, std::vector<std::string> vec) = 0;
        virtual int getConnectShmValue(ConnectShm& connect, std::vector<std::string> vec) = 0;
        virtual int getMoniValue(Moni& moni, std::vector<std::string> vec) = 0;
        virtual int getSessionConfigValue(SessionConfig& conf, std::vector<std::string> vec) = 0;
        virtual int getReportValue(Report& report, std::vector<std::string> vec) = 0;

        virtual int getProxyValue(Proxy& proxy, std::vector<std::string> vec) = 0;
        virtual int getCtrlValue(Ctrl& ctrl, std::vector<std::string> vec) = 0;
        virtual int getWorkerValue(Worker& worker, std::vector<std::string> vec) = 0;

        virtual int getIptableValue(Iptable& iptable, std::vector<std::string> vec) = 0;
        virtual int getAsyncSessionValue(AsyncSession& asyncSession, std::vector<std::string> vec) = 0;

        virtual int getExceptionValue(Exception& exception, std::vector<std::string> vec) = 0;
        virtual int getClusterValue(ClusterConf& cluster, std::vector<std::string> vec) = 0;

		virtual int getCGroupConfValue(CGroupConf& cgroupconf, std::vector<std::string> vec) = 0;

        const std::string& getConfFileName(){return _fileName;}
        std::string trans(const std::vector<std::string>& vec);
    protected:
        std::string _fileName;
    private:

};
} //namespace spp
} //namespace tconfbase
#endif

