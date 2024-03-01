#include "loadxml.h"
#include "singleton.h"

#include <iostream>

using namespace std;

using namespace spp::tconfbase;
using namespace spp::tinyxml;
CLoadXML::CLoadXML(const std::string& filename):CLoadConfBase(filename)
{
    return;
}
CLoadXML::~CLoadXML()
{
    fini();
    return;
}
void CLoadXML::fini()
{
    if (_pDoc != NULL)
    {
        delete _pDoc;
        _pDoc = NULL;
    }
}
int CLoadXML::loadConfFile()
{
    _pDoc = new TiXmlDocument(_fileName.c_str());
    if (!_pDoc->LoadFile())
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, can not load config file.\n", _fileName.c_str());
        delete _pDoc;
        _pDoc = NULL;
        return ERR_LOAD_CONF_FILE;
    }
    return 0;
}
TiXmlElement* CLoadXML::checkBeforeGet(const std::vector<std::string>& vec)
{
    if (_pDoc == NULL)
    {
        LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, init function not load the config file.\n", _fileName.c_str());
        return NULL;
    }
    TiXmlElement* elem = getElemByKey(vec);
    if (elem == NULL)
    {
        //LOG_CONF_SCREEN(LOG_ERROR, "[ERROR]In %s, Not find the corresponding element[%s]\n",
        //    _fileName.c_str(), trans(vec).c_str() );
        return NULL;
    }
    return elem;
}

TiXmlElement* CLoadXML::getElemByKey(const std::vector<std::string>& vec)
{
    if (_pDoc == NULL)
        return NULL;
    TiXmlHandle handle(_pDoc);
    for (size_t i = 0; i < vec.size(); ++ i)
    {
        handle = handle.FirstChild(vec[i].c_str());
    }
    return handle.ToElement();
}

int CLoadXML::getLogValue(Log& log, std::vector<std::string> vec)
{
    TiXmlElement* elem =checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryNumberAttributex<int>(elem, "level", &log.level, log.level);
    QueryNumberAttributex<int>(elem, "type", &log.type, log.type);
    QueryStringAttributex(elem, "prefix", &log.prefix, log.prefix);
    QueryStringAttributex(elem, "path", &log.path, log.path);
    QueryNumberAttributex<int>(elem, "maxfilesize", &log.maxfilesize, log.maxfilesize);
    QueryNumberAttributex<int>(elem, "maxfilenum", &log.maxfilenum, log.maxfilenum);

    return 0;
}

int CLoadXML::getLimitValue(Limit& limit, std::vector<std::string> vec)
{
    TiXmlElement* elem =checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryNumberAttributex<unsigned>(elem, "sendcache", &limit.sendcache, limit.sendcache);

    return 0;
}

int CLoadXML::getAcceptorValue(AcceptorSock& acceptor, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "socket", &acceptor.type, acceptor.type);
    QueryNumberAttributex<bool>(elem, "udpclose", &acceptor.udpclose, acceptor.udpclose);
    QueryNumberAttributex<int>(elem, "maxconn", &acceptor.maxconn, acceptor.maxconn);
    QueryNumberAttributex<int>(elem, "maxpkg", &acceptor.maxpkg, acceptor.maxpkg);
    QueryNumberAttributex<int>(elem, "timeout", &acceptor.timeout, acceptor.timeout);
    vec.push_back("entry");
    elem = checkBeforeGet(vec);

    while (elem)
    {
        AcceptorEntry entry;
        QueryStringAttributex(elem, "type", &entry.type);
        QueryStringAttributex(elem, "if", &entry.ip, entry.ip);
        QueryNumberAttributex<unsigned short>(elem, "port", &entry.port);
        QueryNumberAttributex<int>(elem, "TOS", &entry.TOS, entry.TOS);
        QueryNumberAttributex<int>(elem, "oob", &entry.oob, entry.oob);
        QueryNumberAttributex<bool>(elem, "abstract", &entry.abstract, entry.abstract);
        QueryStringAttributex(elem, "path", &entry.path);
        acceptor.entry.push_back(entry);
        elem = elem->NextSiblingElement("entry");
    }
    return 0;
}

int CLoadXML::getStatValue(Stat& stat, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryStringAttributex(elem, "file", &stat.file, stat.file);
    return 0;
}
int CLoadXML::getProcmonValue(Procmon& procmon, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    vec.push_back("group");
    elem = checkBeforeGet(vec);
    while (elem)
    {
        ProcmonEntry entry;
        QueryNumberAttributex<int>(elem, "id", &entry.id);
        QueryStringAttributex(elem, "basepath", &entry.basepath, entry.basepath);
        QueryStringAttributex(elem, "exe", &entry.exe);
        QueryStringAttributex(elem, "etc", &entry.etc);
        QueryNumberAttributex<int>(elem, "exitsignal", &entry.exitsignal, entry.exitsignal);
        QueryNumberAttributex<unsigned>(elem, "minprocnum", &entry.minprocnum, entry.minprocnum);
        QueryNumberAttributex<unsigned>(elem, "maxprocnum", &entry.maxprocnum, entry.maxprocnum);
        QueryNumberAttributex<unsigned>(elem, "heartbeat", &entry.heartbeat, entry.heartbeat);
        QueryNumberAttributex<unsigned>(elem, "affinity", &entry.affinity);
        QueryNumberAttributex<unsigned>(elem, "reload", &entry.reload);

        procmon.entry.push_back(entry);
        elem = elem->NextSiblingElement("group");
    }

    return 0;
}
int CLoadXML::getModuleValue(Module& module, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "bin", &module.bin);
    QueryStringAttributex(elem, "etc", &module.etc);
    QueryNumberAttributex<bool>(elem, "global", &module.isGlobal, module.isGlobal);
    QueryStringAttributex(elem, "local_handle", &module.localHandleName);
    return 0;
}

int CLoadXML::getResultValue(Result& result, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "bin", &result.bin);
    return 0;
}

int CLoadXML::getConnectShmValue(ConnectShm& connect, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryStringAttributex(elem, "type", &connect.type, connect.type);
    QueryStringAttributex(elem, "scriptname", &connect.scriptname);

    vec.push_back("entry");

    elem = getElemByKey(vec);

    while (elem)
    {
        ConnectEntry entry;
        QueryStringAttributex(elem, "type", &entry.type, entry.type);
        QueryNumberAttributex<int>(elem, "groupid", &entry.id);
        QueryNumberAttributex<int>(elem, "send_size", &entry.send_size, entry.send_size);
        QueryNumberAttributex<int>(elem, "recv_size", &entry.recv_size, entry.recv_size);
        QueryNumberAttributex<unsigned> (elem, "msg_timeout", &entry.msg_timeout, entry.msg_timeout);
        connect.entry.push_back(entry);
        elem = elem->NextSiblingElement("entry");
    }

    return 0;
}
int CLoadXML::getMoniValue(Moni& moni, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryNumberAttributex<int>(elem, "intervial", &moni.intervial, moni.intervial);
    return getLogValue(moni.log, vec);
}

int CLoadXML::getSessionConfigValue(SessionConfig& conf, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "etc", &conf.etc);
    QueryNumberAttributex<int>(elem, "max_epoll_timeout", &conf.epollTime);
    return 0;
}
int CLoadXML::getReportValue(Report& report, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "exe", &report.report_tool);
    return 0;
}
int CLoadXML::getProxyValue(Proxy& proxy, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    
    QueryNumberAttributex<int>(elem, "groupid", &proxy.groupid, proxy.groupid);
    QueryNumberAttributex<int>(elem, "shm_fifo", &proxy.shm_fifo, proxy.shm_fifo);

    return 0;
}
int CLoadXML::getWorkerValue(Worker& worker, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryNumberAttributex<int>(elem, "groupid", &worker.groupid);
    QueryNumberAttributex<int>(elem, "TOS", &worker.TOS, worker.TOS);
    QueryNumberAttributex<int>(elem, "L5us", &worker.L5us, worker.L5us);
    QueryNumberAttributex<int>(elem, "shm_fifo", &worker.shm_fifo, worker.shm_fifo);
    QueryNumberAttributex<int>(elem, "link_timeout_limit", &worker.link_timeout_limit, worker.link_timeout_limit);
    QueryStringAttributex(elem, "proxy", &worker.proxy, worker.proxy);
    return 0;
}
int CLoadXML::getCtrlValue(Ctrl& ctrl, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    return 0;
}
int CLoadXML::getIptableValue(Iptable& iptable, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    QueryStringAttributex(elem, "whitelist", &iptable.whitelist);
    QueryStringAttributex(elem, "blacklist", &iptable.blacklist);

    return 0;
}

int CLoadXML::getAsyncSessionValue(AsyncSession& asyncSession, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }
    vec.push_back("session");
    elem = checkBeforeGet(vec);
    while (elem)
    {
        AsyncSessionEntry entry;
        QueryNumberAttributex<int>(elem, "id", &entry.id);
        QueryNumberAttributex<int>(elem, "port", &entry.port);
        QueryNumberAttributex<int>(elem, "recv_count", &entry.recv_count);
        QueryNumberAttributex<int>(elem, "timeout", &entry.timeout);
        QueryNumberAttributex<int>(elem, "multi_con_sup", &entry.multi_con_sup, entry.multi_con_sup);
        QueryNumberAttributex<int>(elem, "multi_con_inf", &entry.multi_con_inf, entry.multi_con_inf);
        QueryStringAttributex(elem, "type", &entry.maintype);
        QueryStringAttributex(elem, "subtype", &entry.subtype);
        QueryStringAttributex(elem, "ip", &entry.ip);
        asyncSession.entry.push_back(entry);
        elem = elem->NextSiblingElement("session");
    }

    return 0;
}

int CLoadXML::getExceptionValue(Exception& exception, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryNumberAttributex<bool>(elem, "coredump", &exception.coredump);
    QueryNumberAttributex<bool>(elem, "packetdump", &exception.packetdump);	
    QueryNumberAttributex<bool>(elem, "restart", &exception.restart);	
    QueryNumberAttributex<bool>(elem, "monitor", &exception.monitor);
    QueryNumberAttributex<bool>(elem, "realtime", &exception.realtime);        

    return 0;
}

int CLoadXML::getClusterValue(ClusterConf& cluster, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryStringAttributex(elem, "type", &cluster.type, cluster.type);

    return 0;
}


int CLoadXML::getCGroupConfValue(CGroupConf& cgroupconf, std::vector<std::string> vec)
{
    TiXmlElement* elem = checkBeforeGet(vec);
    if (elem == NULL)
    {
        return ERR_NO_SUCH_ELEM;
    }

    QueryNumberAttributex<uint32_t>(elem, "cpu_sys_reserved_percent", &cgroupconf.cpu_sys_reserved_percent);
    QueryNumberAttributex<uint32_t>(elem, "mem_sys_reserved_mb", &cgroupconf.mem_sys_reserved_mb);	
    QueryNumberAttributex<uint32_t>(elem, "cpu_service_percent", &cgroupconf.cpu_service_percent);	
    QueryNumberAttributex<uint32_t>(elem, "mem_service_percent", &cgroupconf.mem_service_percent);
    QueryNumberAttributex<uint32_t>(elem, "cpu_tools_percent", &cgroupconf.cpu_tools_percent); 
    QueryNumberAttributex<uint32_t>(elem, "mem_tools_percent", &cgroupconf.mem_tools_percent); 
    return 0;    
}
