#include <unistd.h>
#include "proxyconf.h"

using namespace spp::tconfbase;
CProxyConf::CProxyConf(CLoadConfBase* pConf)
    :CModuleConfBase(pConf)
{
    return;
}
CProxyConf::~CProxyConf()
{
    return;
}
int CProxyConf::init()
{
    int ret = CModuleConfBase::init();
    if (ret != 0)
        return ret;
    ret = loadProxy(); 
    if (ret != 0)
        return ret;
    ret = checkProxy();
    return ret;
}
int CProxyConf::loadFlog()
{
    _flog.clear("spp_frame_proxy");
    return _pLoadConf->getLogValue(_flog, getArgvs(2, "proxy", "flog"));
}
int CProxyConf::loadFStat()
{
    _fstat.clear("../stat/stat_spp_proxy.dat");
    int ret = _pLoadConf->getStatValue(_fstat, getArgvs(2, "proxy", "fstat"));
    if (ret == 0)
    {
        printf("Proxy[%5d] [WARNING]XML node[fstat] is discarded since 2.8.0\n", getpid());
    }
    return ret;
}
int CProxyConf::loadMoni()
{
    _moni.clear("moni_spp_proxy");
    _moni.log.path = "../moni";
    _moni.log.level = LOG_NORMAL;
    return _pLoadConf->getMoniValue(_moni, getArgvs(2, "proxy", "moni"));
}
int CProxyConf::loadStat()
{
    _stat.clear("../stat/module_stat_spp_proxy.dat");
    return _pLoadConf->getStatValue(_stat, getArgvs(2, "proxy", "stat"));
} 
int CProxyConf::loadLog()
{
    _log.clear("spp_proxy");
    return _pLoadConf->getLogValue(_log, getArgvs(2, "proxy", "log"));
}

int CProxyConf::loadLimit()
{
    _limit.clear();
    return _pLoadConf->getLimitValue(_limit, getArgvs(2, "proxy", "limit"));
}
int CProxyConf::loadAcceptor()
{
    _acceptor.clear();
    int ret = _pLoadConf->getAcceptorValue(_acceptor, getArgvs(2, "proxy", "acceptor"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[acceptor]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}
int CProxyConf::loadConnector()
{
    _connector.clear();
    int ret = _pLoadConf->getConnectShmValue(_connector, getArgvs(2, "proxy", "connector"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[connector]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}
int CProxyConf::loadModule()
{
    _module.clear();
    int ret = _pLoadConf->getModuleValue(_module, getArgvs(2, "proxy", "module"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[module]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}

int CProxyConf::loadResult()
{
    _result.clear();
    return _pLoadConf->getResultValue(_result, getArgvs(2, "proxy", "result"));
}

int CProxyConf::loadProxy()
{
    _proxy.clear();
    int ret = _pLoadConf->getProxyValue(_proxy, getArgvs(1, "proxy"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[proxy]\n", _pLoadConf->getConfFileName().c_str() );
    }
    return ret;
}
int CProxyConf::loadIptable()
{
    _iptable.clear();
    return _pLoadConf->getIptableValue(_iptable, getArgvs(2, "proxy", "iptable"));
}

int CProxyConf::loadException()
{
    _exception.clear(); 
    return _pLoadConf->getExceptionValue(_exception, getArgvs(2, "proxy", "exception"));
}

int CProxyConf::checkStat()
{
    return 0;    
}

int CProxyConf::checkLimit()
{
    return 0;    
}
int CProxyConf::checkLog()
{
    return checklog("log", _log);
}
int CProxyConf::checkAcceptor()
{
    if (_acceptor.type != "socket")
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, the proxy acceptor type must be \"socket\".\n", _pLoadConf->getConfFileName().c_str());
        return ERR_CONF_CHECK_UNPASS;
    }
    if (_acceptor.maxconn <=0 || _acceptor.maxpkg < 0 || _acceptor.timeout < 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, please check<acceptor>:\n"
                "* acceptor maxconn:%d should be > 0\n"
                "* acceptor maxpkg:%d should be >= 0\n"
                "* acceptor timeout:%d should be >= 0\n",
                _pLoadConf->getConfFileName().c_str(),
                _acceptor.maxconn,
                _acceptor.maxpkg,
                _acceptor.timeout
                );
        return ERR_CONF_CHECK_UNPASS;
    }
    if (_acceptor.entry.size() == 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, the acceptor must have at least 1 entry.\n", _pLoadConf->getConfFileName().c_str());
        return ERR_CONF_CHECK_UNPASS;
    }
    for (size_t i = 0; i < _acceptor.entry.size(); ++ i)
    {
        if (_acceptor.entry[i].type != "tcp" 
            && _acceptor.entry[i].type != "udp"
            && _acceptor.entry[i].type != "unix")
        {
            LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, the acceptor entry type can only be tcp/udp/unix, but not %s\n", _pLoadConf->getConfFileName().c_str(), _acceptor.entry[i].type.c_str());
            return ERR_CONF_CHECK_UNPASS;
        }
        if (_acceptor.entry[i].type == "tcp" || _acceptor.entry[i].type == "udp")
        {
            if (_acceptor.entry[i].TOS < -1 || _acceptor.entry[i].TOS > 255)
            {
                LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, the acceptor entry TOS should not < -1 or > 255.\n" , _pLoadConf->getConfFileName().c_str());
                return ERR_CONF_CHECK_UNPASS;
            } 
        }
    
    }
        
    return 0;
}
int CProxyConf::checkConnector()
{
    if (_connector.type != "shm" && _connector.type != "local")
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, please check:the proxy connector type is %s now.\n", _pLoadConf->getConfFileName().c_str(), _connector.type.c_str());
        return ERR_CONF_CHECK_UNPASS;
    }
    if (_connector.entry.size() == 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]Proxy should have at least 1 connector.\n");
        return ERR_CONF_CHECK_UNPASS;
    }
    for (size_t i = 0; i < _connector.entry.size(); ++ i)
    {
        if (_connector.entry[i].type != "shm" && _connector.entry[i].type != "local")
        {
            LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, the connector entry type must be \"shm or local\"\n", _pLoadConf->getConfFileName().c_str());
            return ERR_CONF_CHECK_UNPASS;
        }
    }
    return 0;
}
int CProxyConf::checkModule()
{
    return 0;
}
int CProxyConf::checkProxy()
{
    if (_proxy.groupid < 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, groupid:%d should be >= 0.\n", _pLoadConf->getConfFileName().c_str(), _proxy.groupid);
        return ERR_CONF_CHECK_UNPASS;
    }
    return 0;
}
int CProxyConf::checkIptable()
{
    if (_iptable.whitelist != "")
    {
        if (_iptable.blacklist != "")
        {
            LOG_CONF_SCREEN(LOG_ERROR, "white iptable already loaded,black iptable is invalid.\n");
        }
    }
    return 0;
}

int CProxyConf::checkFStat()
{
    _fstat.file = "../stat/stat_spp_proxy.dat";
    return 0;
}

int CProxyConf::getAndCheckFlog(Log& flog)
{
    int ret = loadFlog();
    if (ret != 0)
    {
        flog = _flog;
        return ret;
    }
    ret = checkFlog();
    flog = _flog;
    return ret;
}

int CProxyConf::getAndCheckLog(Log& log)
{
    int ret = loadLog();
    if (ret != 0)
    {
        log  = _log;
        return ret;
    }
    ret = checkLog();
    log = _log;
    return ret;
}
int CProxyConf::getAndCheckStat(Stat& stat)
{
    int ret = loadStat();
    if (ret != 0)
    {
        stat = _stat;
        return ret;
    }
    ret = checkStat();
    stat = _stat;
    return ret; 
}

int CProxyConf::getAndCheckLimit(Limit& limit)
{
    int ret = loadLimit();
    if (ret != 0)
    {
        limit = _limit;
        return ret;
    }

    ret = checkLimit();
    limit = _limit;
    return ret;
}
int CProxyConf::getAndCheckFStat(Stat& fstat)
{
    int ret = loadFStat();
    if (ret != 0)
    {
        fstat = _fstat;
        return ret ;
    }
    ret = checkFStat();
    fstat = _fstat;
    return ret;
}
int CProxyConf::getAndCheckMoni(Moni& moni)
{
    int ret = loadMoni();
    if (ret != 0)
    {
        moni = _moni;
        return ret;
    }
    ret = checkMoni();
    moni = _moni;
    return ret;
}
int CProxyConf::getAndCheckAcceptor(AcceptorSock& acceptor)
{
    int ret = loadAcceptor();
    if (ret != 0)
    {
        acceptor = _acceptor;
        return ret;
    }
    ret = checkAcceptor();
    acceptor = _acceptor;
    return ret;
}
int CProxyConf::getAndCheckConnector(ConnectShm& connector)
{
    int ret = loadConnector();
    if (ret != 0)
    {
        connector = _connector;
        return ret;
    }
    ret = checkConnector();
    connector = _connector;
    return ret;
}
int CProxyConf::getAndCheckModule(Module& module)
{
    int ret = loadModule();
    if (ret != 0)
    {
        module = _module;
        return ret;
    }
    ret = checkModule();
    module = _module;
    return ret;
}
int CProxyConf::getAndCheckProxy(Proxy& proxy)
{
    int ret = loadProxy();
    if (ret != 0)
    {
        proxy = _proxy;
        return ret;
    }
    ret = checkProxy();
    proxy = _proxy;
    return ret;
}
int CProxyConf::getAndCheckIptable(Iptable& iptable)
{
    int ret = loadIptable();
    if (ret != 0)
    {
        iptable = _iptable;
        return ret;
    }
    ret = checkIptable();
    iptable = _iptable;
    return ret;
}

int CProxyConf::getAndCheckException(Exception& exception)
{
    int ret = loadException();

    exception = _exception;

    return ret;
}

int CProxyConf::getAndCheckResult(Result& result)
{
    int ret = loadResult();
    result = _result;
    return ret;
}

			
