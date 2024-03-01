#ifndef _HTTP_GLOBAL_H_
#define _HTTP_GLOBAL_H_

#include <malloc.h>
#include <unistd.h>
#include <stdint.h>
#include "defs.h"


class TGlobal
{
public: //method
    static void ShowVersion(void);
    static void ShowUsage(void);
    static int DaemonInit(const char* config_file);
    static int DaemonStart(void);
    static void DaemonWait(void);
    static void DaemonCleanup(void);

public: //property
    static volatile int*    module_close;
    static volatile int     stopped;
    static int              accesslog;
    static const char*      progname;
    static const char*      version;
    static int              background;
    static char             httpsvr_conf_file[256];

public:
    static char            _addr[64];
    static uint16_t        _port;
    static int             _backlog;
    static int             _maxpoller;
    static int             _idletimeout;
    static int		   _reconnect_interval;
    static int             _helpertimeout;
    static int             _acceptcnt;
    static int             _cleancycle;
    static int             _usechunk;

private: //no impl
    TGlobal(void);
    ~TGlobal(void);
    TGlobal(const TGlobal&);
    const TGlobal& operator= (const TGlobal&);

private: //method
    static void update_handler(int signo);
};


#endif
