#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>

#include <global.h>
#include "spp_async_def.h"

using namespace std;

//HTTP_SVR_NS_BEGIN

volatile int*   TGlobal::module_close            = NULL;
volatile int    TGlobal::stopped                 = 0;
int             TGlobal::accesslog               = 0;
const char*     TGlobal::progname                = "qzhttp proxy";
const char*     TGlobal::version                 = "version: 2.1.0 date: 2008-08-07";
int             TGlobal::background              = 1;
char            TGlobal::httpsvr_conf_file[256]  = {'\0'};

char            TGlobal::_addr[64]               = "0.0.0.0";
uint16_t        TGlobal::_port                   = 80;
int             TGlobal::_backlog                = 256;
int             TGlobal::_maxpoller              = 100000;		//async spp used
int             TGlobal::_idletimeout             = 0;			//async spp used, unit ms
int             TGlobal::_helpertimeout          = 5;
int             TGlobal::_reconnect_interval     = DEFAULT_RECONNECT_INTERVAL; 		//async spp used
int             TGlobal::_acceptcnt              = 256;
int             TGlobal::_cleancycle             = 1;
int             TGlobal::_usechunk               = 1;

void TGlobal::ShowVersion()
{
    printf("%s v%s\n", progname, version);
}

void TGlobal::ShowUsage()
{
    ShowVersion();
    printf("Usage:\n    %s [-d] [-f httpsvr.conf] [-x cgiconfig.xml]\n", progname);
}

int TGlobal::DaemonInit(const char* config_file)
{
    return 0;
}

void TGlobal::update_handler(int signo)
{
}

void TGlobal::DaemonWait(void)
{
    while (!stopped) {
        sleep(5000);

#if MEMCHECK
        log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());
#endif
    }
}

int TGlobal::DaemonStart(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = update_handler;
    sigaction(SIGUSR2, &sa, NULL);

    return 0;
}

void TGlobal::DaemonCleanup(void)
{
}


//HTTP_SVR_NS_END
