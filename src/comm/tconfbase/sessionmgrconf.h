#ifndef _SPP_SESSIONMGRCONF_H__
#define _SPP_SESSIONMGRCONF_H__
#include "moduleconfbase.h"
#include "loadxml.h"
namespace spp
{
namespace tconfbase
{
class CSessionMgrConf: public CModuleConfBase
{
    public:
        CSessionMgrConf(CLoadConfBase* pConf);
        virtual ~CSessionMgrConf();

        int init();

        int getAndCheckAsyncSession(AsyncSession& session);
        AsyncSession getAsyncSession(){return _session;};
       
    protected:
        int loadAsyncSession();
        int checkAsyncSession();

    private:
        int loadFlog(){return 0;};
        int loadFStat(){return 0;};
        int loadMoni(){return 0;};
        AsyncSession _session;




};
}
}
#endif
