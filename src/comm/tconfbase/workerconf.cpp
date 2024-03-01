#include <unistd.h>
#include "workerconf.h"

using namespace spp::tconfbase;
CWorkerConf::CWorkerConf(CLoadConfBase* pConf)
    :CModuleConfBase(pConf)
{
    return;
}
CWorkerConf::~CWorkerConf()
{
    return;
}
int CWorkerConf::init()
{
    int ret = 0;
    ret = CModuleConfBase::init();
    if (ret != 0 )
        return ret;
    ret = loadWorker();
    if (ret != 0)
        return ret;
    ret = checkWorker();
    memset(workername, 0, 64);
    snprintf(workername, 64, "worker%d", _worker.groupid);
    return ret;
}
int CWorkerConf::loadFlog()
{
    std::string prefix("spp_frame_");
    prefix += workername;
    _flog.clear(prefix);
    return _pLoadConf->getLogValue(_flog, getArgvs(2, "worker", "flog"));
}
int CWorkerConf::loadFStat()
{
    std::string file = "../stat/stat_spp_";
    file += workername;
    file += ".dat";
    _fstat.clear(file);
    int ret = _pLoadConf->getStatValue(_fstat, getArgvs(2, "worker", "fstat"));
    if (ret == 0)
    {
        printf("Worker[%5d] [WARNING]XML node[fstat] is discarded since 2.8.0\n", getpid());
    }
    return ret;
}
int CWorkerConf::loadMoni()
{
    _moni.clear();
    return _pLoadConf->getMoniValue(_moni, getArgvs(2, "worker", "moni"));
}
int CWorkerConf::loadStat()
{
    std::string file = "../stat/module_stat_spp_";
    file += workername;
    file += ".dat";
    _stat.clear(file);
    return _pLoadConf->getStatValue(_stat, getArgvs(2, "worker", "stat"));
} 

int CWorkerConf::loadLog()
{
    std::string prefix("spp_");
    prefix += workername;

    _log.clear(prefix);
    return _pLoadConf->getLogValue(_log, getArgvs(2, "worker", "log"));
}

int CWorkerConf::loadAcceptor()
{
    _acceptor.clear();
    return _pLoadConf->getConnectShmValue(_acceptor, getArgvs(2, "worker", "acceptor"));
}
int CWorkerConf::loadModule()
{
    _module.clear();
    int ret = _pLoadConf->getModuleValue(_module, getArgvs(2, "worker", "module"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[module]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}
int CWorkerConf::loadWorker()
{
    _worker.clear();
    int ret = _pLoadConf->getWorkerValue(_worker, getArgvs(1, "worker"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[worker]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}
int CWorkerConf::loadSessionConfig()
{
   _session.clear(); 
   return _pLoadConf->getSessionConfigValue(_session, getArgvs(2, "worker", "session_config"));
}

int CWorkerConf::loadException()
{
    _exception.clear(); 
    return _pLoadConf->getExceptionValue(_exception, getArgvs(2, "worker", "exception"));
}

int CWorkerConf::checkStat()
{
    return 0;    
}
int CWorkerConf::checkLog()
{
    return checklog("log", _log);
}
int CWorkerConf::checkAcceptor()
{
    if (_acceptor.entry.size () == 0)
    {
        _acceptor.entry.push_back(ConnectEntry());
    }
    else
    {
        if (_acceptor.entry[0].msg_timeout != 0 && _acceptor.entry[0].msg_timeout < 10)
        {
            _acceptor.entry[0].msg_timeout = 10;
        }
    }
    return 0;
}
int CWorkerConf::checkModule()
{
    return 0;
}
int CWorkerConf::checkSessionConfig()
{
    return 0;
}
int CWorkerConf::checkWorker()
{
    if (_worker.groupid <= 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]in %s, groupid:%d must > 0\n", _pLoadConf->getConfFileName().c_str(), _worker.groupid);
        return ERR_CONF_CHECK_UNPASS;
    }
    if (_worker.TOS < -1 || _worker.TOS > 255)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, TOS should not < -1 or > 255",
               _pLoadConf->getConfFileName().c_str()); 
        return ERR_CONF_CHECK_UNPASS;
    } 
    return 0;
}


int CWorkerConf::checkFStat()
{

    std::string file = "../stat/stat_spp_";
    file += workername;
    file += ".dat";
    _fstat.file = file;
    return 0;
}

int CWorkerConf::getAndCheckFlog(Log& flog)
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

int CWorkerConf::getAndCheckLog(Log& log)
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
int CWorkerConf::getAndCheckStat(Stat& stat)
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
int CWorkerConf::getAndCheckMoni(Moni& moni)
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
int CWorkerConf::getAndCheckAcceptor(ConnectShm& acceptor)
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
int CWorkerConf::getAndCheckModule(Module& module)
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
int CWorkerConf::getAndCheckFStat(Stat& fstat)
{
    int ret = loadFStat();
    if (ret != 0)
    {
        fstat = _fstat;
        return ret;
    }
    ret = checkFStat();
    fstat = _fstat; 
    return ret;
}
int CWorkerConf::getAndCheckSessionConfig(SessionConfig& session)
{
    int ret = loadSessionConfig();
    if (ret != 0)
    {
        session = _session;
        return ret;
    }
    ret = checkSessionConfig();
    session = _session;
    return ret;        
}

int CWorkerConf::getAndCheckException(Exception& exception)
{
    int ret = loadException();

    exception = _exception;

    return ret;
}

