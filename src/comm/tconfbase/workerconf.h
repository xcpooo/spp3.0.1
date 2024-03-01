#ifndef _SPP_WORKERCONF_H__
#define _SPP_WORKERCONF_H__
#include "moduleconfbase.h"
#include "loadxml.h"
namespace spp
{
namespace tconfbase
{
class CWorkerConf : public CModuleConfBase
{
    public:
        CWorkerConf(CLoadConfBase* pConf);
        virtual ~CWorkerConf();

        int init();

        int getAndCheckFlog(Log& flog);
        int getAndCheckFStat(Stat& fstat);
        int getAndCheckMoni(Moni& moni);
        int getAndCheckStat(Stat& stat);
        int getAndCheckLog(Log& log);
        int getAndCheckAcceptor(ConnectShm& acceptor);
        int getAndCheckModule(Module& module);
        int getAndCheckSessionConfig(SessionConfig& session);
		int getAndCheckException(Exception& exception);
        
        Stat getStat(){return _stat;};
        Log getLog(){return _log;};
        ConnectShm getAcceptor(){return _acceptor;};
        Module getModule(){return _module;};
        SessionConfig getSessionConfig(){return _session;};
        Worker getWorker(){return _worker;};
       
    protected:
        int loadFlog();
        int loadFStat();
        int loadMoni();
        int loadStat();
        int loadLog();
        int loadAcceptor();
        int loadModule();
        int loadSessionConfig();
        int loadWorker();
		int loadException();
        
        int checkStat();
        int checkLog();
        int checkAcceptor();
        int checkModule();
        int checkSessionConfig();
        int checkWorker();
        int checkFStat();

    private:
        Stat _stat;
        Log _log;
        Module _module;
        Worker _worker;
        ConnectShm _acceptor;
        SessionConfig _session;
		
		Exception _exception;

        char workername[64] ;


};
}
}
#endif
