#include <unistd.h>
#include "sessionmgrconf.h"

using namespace spp::tconfbase;
CSessionMgrConf::CSessionMgrConf(CLoadConfBase* pConf)
        :CModuleConfBase(pConf)
{
    return;
}
CSessionMgrConf::~CSessionMgrConf()
{
    return;
}
int CSessionMgrConf::init()
{
    int ret = 0;
    ret = CModuleConfBase::init();
    return ret;
}
int CSessionMgrConf::getAndCheckAsyncSession(AsyncSession& session)
{
   int ret = loadAsyncSession();
   if (ret != 0)
   {
       session = _session;
       return ret;
   } 
   ret = checkAsyncSession();
   session = _session; 
   return ret;
}
int CSessionMgrConf::loadAsyncSession()
{
    _session.clear();
    int ret = _pLoadConf->getAsyncSessionValue(_session, getArgvs(1, "session_group"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, " Load session file[%s]:find no session_group\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
    
}
int CSessionMgrConf::checkAsyncSession()
{
    return 0;
}
