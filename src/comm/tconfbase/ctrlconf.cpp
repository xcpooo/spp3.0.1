#include "ctrlconf.h"

using namespace spp::tconfbase;

CCtrlConf::CCtrlConf(CLoadConfBase* pConf):CModuleConfBase(pConf)
{
    return;
}
CCtrlConf::~CCtrlConf()
{
    return;
}
int CCtrlConf::init()
{
    int ret = 0;
    ret = CModuleConfBase::init();
    if (ret != 0)
        return ret;
    ret = loadCtrl();
    if (ret != 0)
        return ret;
    return checkCtrl();
}

int CCtrlConf::loadFlog()
{
    _flog.clear("spp_frame_ctrl");
    return _pLoadConf->getLogValue(_flog, getArgvs(2, "controller", "flog"));
}
int CCtrlConf::loadFStat()
{
    _fstat.clear("../stat/stat_spp_ctrl.dat");
    return _pLoadConf->getStatValue(_fstat, getArgvs(2, "controller", "fstat"));
}

int CCtrlConf::loadMoni()
{
    _moni.clear("moni_spp_ctrl");
    _moni.log.path = "../moni";
    return _pLoadConf->getMoniValue(_moni, getArgvs(2, "controller", "moni"));
}
int CCtrlConf::loadReport()
{
    _report.clear();
    return _pLoadConf->getReportValue(_report, getArgvs(2, "controller", "report"));
}
int CCtrlConf::loadProcmon()
{
    _procmon.clear();
    int ret = _pLoadConf->getProcmonValue(_procmon, getArgvs(2, "controller", "procmon"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[procmon]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}
int CCtrlConf::loadCtrl()
{
    _ctrl.clear(); 
    int ret = _pLoadConf->getCtrlValue(_ctrl, getArgvs(1, "controller"));
    if (ret != 0)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the element[controller]\n", _pLoadConf->getConfFileName().c_str());
    }
    return ret;
}

int CCtrlConf::loadException()
{
    _exception.clear(); 
    return _pLoadConf->getExceptionValue(_exception, getArgvs(2, "controller", "exception"));
}

int CCtrlConf::checkReport()
{
    return 0;
}
int CCtrlConf::checkProcmon()
{
    for (size_t i = 0; i < _procmon.entry.size(); ++ i)
    {
        if (_procmon.entry[i].exe == "")
        {
            LOG_CONF_SCREEN(LOG_ERROR, "In %s, group id:%d exe cannot not be null.\n", _pLoadConf->getConfFileName().c_str(), _procmon.entry[i].id);
            return ERR_CONF_CHECK_UNPASS;
        }
        if (_procmon.entry[i].affinity < 0)
        {
            _procmon.entry[i].affinity = 0;
        }
        if (_procmon.entry[i].minprocnum < 0 ||
            _procmon.entry[i].minprocnum > _procmon.entry[i].maxprocnum ||
            _procmon.entry[i].exitsignal <= 0 ||
            _procmon.entry[i].id < 0)
        {
            LOG_CONF_SCREEN(LOG_ERROR, "In %s,please check:\n"
                    "* groupid should be >= 0, now groupid=%d\n"
                    "* minprocnum should be >= 0 and <= maxprocnum, now minprocnum=%d, maxprocnum=%d\n"
                    "* exitsignal should be > 0, now exitsignal=%d\n",
                    _pLoadConf->getConfFileName().c_str(),
                    _procmon.entry[i].id,
                    _procmon.entry[i].minprocnum,
                    _procmon.entry[i].maxprocnum,
                    _procmon.entry[i].exitsignal
                    );
            return ERR_CONF_CHECK_UNPASS;
        }
    }
    return 0;
}
int CCtrlConf::checkFlog()
{
    //忽略用户配置的路径, 日志文件前缀, 日志类型
    _flog.type = LOG_TYPE_CYCLE;
    _flog.path = "../log";
    _flog.prefix = "spp_frame_ctrl";
    return checklog("flog", _flog);
}
int CCtrlConf::checkCtrl()
{
    return 0;
}

int CCtrlConf::getAndCheckFlog(Log& flog)
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
int CCtrlConf::getAndCheckFStat(Stat& fstat)
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
int CCtrlConf::getAndCheckMoni(Moni& moni)
{
    int ret = loadMoni();
    if (ret != 0)
    {
        moni = _moni;
        return ret;
    }
    ret = checkMoni();
    moni =  _moni;
    return ret;
}
int CCtrlConf::getAndCheckReport(Report& report)
{
    int ret = loadReport();
    if (ret != 0)
    {
        report = _report;
        return ret;
    }
    ret = checkReport();
    report = _report;
    return ret;

}
int CCtrlConf::getAndCheckProcmon(Procmon& procmon)
{
    int ret = loadProcmon();
    if (ret != 0)
    {
        procmon = _procmon;
        return ret;
    }
    ret = checkProcmon();
    procmon = _procmon;
    return ret;
}

int CCtrlConf::getAndCheckException(Exception& exception)
{
    int ret = loadException();

    exception = _exception;

    return ret;
}

