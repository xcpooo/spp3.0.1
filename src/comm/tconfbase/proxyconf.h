#ifndef _SPP_PROXYCONF_H__
#define _SPP_PROXYCONF_H__
#include "moduleconfbase.h"
#include "loadxml.h"
namespace spp
{
namespace tconfbase
{
class CProxyConf : public CModuleConfBase
{
    public:
        CProxyConf(CLoadConfBase* pConf);
        virtual ~CProxyConf();
        
        int init();

        int getAndCheckFlog(Log& flog);
        int getAndCheckLog(Log& log);
        int getAndCheckLimit(Limit& limit);
        int getAndCheckStat(Stat& stat);
        int getAndCheckFStat(Stat& fstat);
        int getAndCheckMoni(Moni& moni);
        int getAndCheckAcceptor(AcceptorSock& acceptor);
        int getAndCheckConnector(ConnectShm& connector);
        int getAndCheckModule(Module& module);
        int getAndCheckProxy(Proxy& proxy);
        int getAndCheckIptable(Iptable& iptable);
		int getAndCheckException(Exception& exception);
        int getAndCheckResult(Result& result);

        const Stat& getStat(){return _stat;};
        const Log& getLog(){return _log;};
        const AcceptorSock& getAcceptor(){return _acceptor;};
        const ConnectShm& getConnector(){return _connector;};
        const Module& getModule(){return _module;};
        const Proxy& getProxy(){return _proxy;};
        const Iptable& getIptable(){return _iptable;};
       
    protected:
        int loadFlog();
        int loadFStat();
        int loadMoni();
        int loadStat();
        int loadLog();
        int loadLimit();
        int loadAcceptor();
        int loadConnector();
        int loadModule();
        int loadProxy();
        int loadIptable();
		int loadException();
        int loadResult();
        
        int checkStat();
        int checkLimit();
        int checkLog();
        int checkAcceptor();
        int checkConnector();
        int checkModule();
        int checkProxy();
        int checkIptable();
        int checkFStat();


    private:
        Stat _stat;
        Log _log;
        AcceptorSock _acceptor;
        ConnectShm _connector;
        Module _module;		
        Result _result;
        Proxy _proxy;
        Iptable _iptable;
		Exception _exception;
        Limit _limit;

};
}
}
#endif
