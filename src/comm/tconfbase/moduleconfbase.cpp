#include "moduleconfbase.h"
using namespace spp::tconfbase;

CModuleConfBase::CModuleConfBase(CLoadConfBase* pConf)
{
    _pLoadConf = pConf;
}
CModuleConfBase::~CModuleConfBase()
{
    if (_pLoadConf != NULL)
    {
        delete _pLoadConf;
        _pLoadConf = NULL;
    }
}
int CModuleConfBase::init()
{
    return _pLoadConf->loadConfFile(); 
}

std::vector<std::string> CModuleConfBase::getArgvs(int count, ...) 
{                                                                
    std::vector<std::string> vec;                                
    va_list argv;                                                
    va_start(argv, count);                                       
    while (--count >= 0)                                         
    {                                                            
        vec.push_back(va_arg(argv, char*));                      
    }                                                            
    va_end(argv);                                                
    return vec;                                                  
}                                                                
int CModuleConfBase::checklog(const std::string& key, const Log& log)
{
    if ((log.level< LOG_TRACE) || (log.level> LOG_NONE) ||
        (log.type < LOG_TYPE_CYCLE) || (log.type > LOG_TYPE_CYCLE_HOURLY) ||
        (log.maxfilesize<= 0)  || (log.maxfilenum<= 0))
    {
        LOG_CONF_SCREEN(LOG_ERROR,"[ERROR]In %s, please check <%s>:\n"
                "* log level:%d should be >= %d and <= %d \n"
                "* log type:%d should be >= %d and <= %d \n"
                "* log maxfilesize:%d should be > 0 \n"
                "* log maxfilenum:%d should be > 0 \n",
                _pLoadConf->getConfFileName().c_str(),
                key.c_str(),
                log.level,
                LOG_TRACE,
                LOG_NONE,
                log.type,
                LOG_TYPE_CYCLE,
                LOG_TYPE_CYCLE_HOURLY,
                log.maxfilesize,
                log.maxfilenum 
                
              );
        return ERR_CONF_CHECK_UNPASS;
    }
    return 0;
}
int CModuleConfBase::checkFlog()
{
    return checklog("flog", _flog);
}

int CModuleConfBase::checkFStat()
{
    return 0;
}
int CModuleConfBase::checkMoni()
{
    if (_moni.intervial <= 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR,"[ERROR]In %s, please check <moni>:\n"
                "* moni intervial should be > 0, now moni intervial=%d\n",
                _pLoadConf->getConfFileName().c_str(),
                _moni.intervial
                );
        return ERR_CONF_CHECK_UNPASS;
    }
    return checklog("moni", _moni.log);
}
