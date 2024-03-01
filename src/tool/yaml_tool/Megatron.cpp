#include "tinyxml.h"
#include "Megatron.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

using namespace std;
using namespace spp::tinyxml;

#define addYAMLKeyValue(key, value) YAML::Key<<key<<YAML::Value<<value

std::string g_sub_type = "spp";
static int g_cpu_num = 4;

static int ExtractCpuNum()
{
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        cerr<< "open /proc/stat failed!"<<endl;
        return -1;
    }

    char line[256];
    g_cpu_num = 0;
    while (fgets(line, sizeof(line)-1, fp))
    {
        // cpu[0-9]
        if (strstr(line, "cpu"))
        {
            if (line[3] <= '9' && line[3] >= '0')
            {
                ++g_cpu_num;
                continue;
            }
        }
        else // ??1?×￠×??°????DD′?cpuμ?D??￠
        {
            break;
        }
    }

    if (!g_cpu_num)
    {
        cerr<<"extract cpu num failed!"<<endl;
        fclose(fp);
        return -2;
    }

    cerr<<"extract cpu num: "<<g_cpu_num<<"!"<<endl;

    fclose(fp);

    return 0;
}

void operator >> (const YAML::Node& node, Listen& listen)
{
    const YAML::Node *pConf = NULL;
    pConf = node.FindValue("address");
    if(NULL != pConf)
        *pConf >> listen.address;
}

void operator >> (const YAML::Node& node, Global& global)
{
    try{
        const YAML::Node *pConf = NULL;
        pConf = node.FindValue("type");
        if(NULL == pConf) // service.yaml
        {
            const YAML::Node& listensNode = node["listen"];
            if(listensNode.size() == 0)
            {
                throw 2;
            }
            for(unsigned i=0;i<listensNode.size();i++) {
                Listen listen;
                listensNode[i] >> listen;
                global.listens.push_back(listen);
            }

            pConf = node.FindValue("limit");
            if (NULL != pConf)
            {
                const YAML::Node& limitNode = *pConf;
                pConf = limitNode.FindValue("sendcache");
                if (NULL != pConf)
                    *pConf >> global.sendcache_limit;
            }
            pConf = node.FindValue("timeout");
            if(NULL != pConf)
            {
                *pConf >> global.timeout;
            }
            pConf = node.FindValue("udpclose");
            if(NULL != pConf)
            {
                *pConf >> global.udpclose;
            }
            pConf = node.FindValue("TOS");
            if(NULL != pConf)
            {
                *pConf >> global.TOS;
            }
            pConf = node.FindValue("maxconn");
            if(NULL != pConf)
            {
                *pConf >> global.maxconn;
            }
            pConf = node.FindValue("whitelist");
            if (NULL != pConf)
            {
                *pConf >> global.whitelist;
            }
            pConf = node.FindValue("blacklist");
            if (NULL != pConf)
            {
                *pConf >> global.blacklist;
            }
            pConf = node.FindValue("L5us");
            if(NULL != pConf)
            {
                *pConf >> global.L5us;
            }

            pConf = node.FindValue("shm_fifo");
            if(NULL != pConf)
            {
                *pConf >> global.shm_fifo;
            }

            pConf = node.FindValue("link_timeout_limit");
            if(NULL != pConf)
            {
               *pConf >> global.link_timeout_limit;
            }
            
        }
        else // spp.yaml
        {
            std::string type;
            node["type"] >> type;
            if(type == "spp")
            {
                node["report"] >> global.report;

                pConf = node.FindValue("proxy");
                if (NULL != pConf)
                {
                    *pConf >> global.proxy_ip;
                }

                pConf = node.FindValue("coredump");
                if (NULL != pConf)
                {
                    *pConf >> global.coredump;
                }

				pConf = node.FindValue("restart");
                if (NULL != pConf)
                {
                    *pConf >> global.restart;
                }
                global.monitor = true;
				pConf = node.FindValue("monitor");
                if (NULL != pConf)
                {
                    *pConf >> global.monitor;
                }

                // 检查框架子类型
                pConf = node.FindValue("subtype");
                if (NULL != pConf)
                {
                    *pConf >> global.sub_type;
                }

                if (global.sub_type != "sf2")
                {
                    global.sub_type = "spp";
                }

                g_sub_type = global.sub_type;
                
                global.realtime=true;
                pConf = node.FindValue("realtime");
                if(NULL!=pConf)
                {
                    *pConf >> global.realtime;
                }
            }
        }
    }
    catch(int)
    {
        cerr << "parse yaml error, listen is empty" << endl;
        exit(-1);
    }
}

void operator >> (const YAML::Node& node, Log& log)
{
    const YAML::Node *pConf = NULL;
    pConf = node.FindValue("level");
    if(NULL != pConf)
        *pConf >> log.level;
    pConf = node.FindValue("type");
    if(NULL != pConf)
        *pConf >> log.type;
    pConf = node.FindValue("maxfilesize");
    if(NULL != pConf)
        *pConf >> log.maxfilesize;
    pConf = node.FindValue("maxfilenum");
    if(NULL != pConf)
        *pConf >> log.maxfilenum;
}

void operator >> (const YAML::Node& node, Service& service)
{
    const YAML::Node *pConf = NULL;
    node["id"] >> service.id;
    node["module"] >> service.module;
	
	if (service.id != 0 || node.FindValue("global"))
    { 
        if(NULL!=node.FindValue("procnum"))
        {
            node["procnum"] >> service.procnum;
        }
        else if(NULL!=node.FindValue("procnumprecore"))
        {
	        node["procnumprecore"] >> service.procnum;
            service.procnum *= g_cpu_num;
        }
    }
    pConf = node.FindValue("global");
    if (NULL != pConf)
    {
        node["global"] >> service.global;
    }
    pConf = node.FindValue("heartbeat");
    if(NULL != pConf)
        node["heartbeat"] >> service.heartbeat;
    service.reload=true;
    pConf = node.FindValue("reload");
    if(NULL != pConf)
        node["reload"] >> service.reload;
    pConf = node.FindValue("conf");
    if(NULL != pConf)
        *pConf >> service.conf;
    pConf = node.FindValue("flog");
    if(NULL != pConf)
    {
        *pConf >> service.flog;
    }
    pConf = node.FindValue("log");
    if(NULL != pConf)
    {
        *pConf >> service.log;
    }
    pConf = node.FindValue("shmsize");
    if(NULL != pConf)
    {
        *pConf >> service.shmsize;
    }
    pConf = node.FindValue("timeout");
    if(NULL != pConf)
    {
        *pConf >> service.timeout;
    }
    else if (g_sub_type == "sf2")
    {
        service.timeout = 800;
    }
    
    pConf = node.FindValue("type");
    if(NULL != pConf)
    {
        *pConf >> service.type;
    }    

    //davisxie
    pConf = node.FindValue("packetdump");
    if(NULL != pConf)
    {
        *pConf >> service.packetdump;
    }

    pConf = node.FindValue("result");
    if(NULL != pConf)
    {
        *pConf >> service.result;
    }
}

void operator >> (const YAML::Node& node, Config& config)
{
    const YAML::Node *pConf = NULL;
    pConf = node["global"].FindValue("type");
    if(NULL != pConf)
    {
        std::string type;
        node["global"]["type"] >> type;
        if(type == "spp")
        {
            try {
                node["global"] >> config.global;
            }
            catch(int){
                cerr << "parse spp.yaml exception" << endl;
                exit(-1);
            }
        }
    }
    else
    {
        try {
            node["global"] >> config.global;

            const YAML::Node& servicesNode = node["service"];
            if(servicesNode.size() == 0)
            {
                throw 1;
            }
            for(unsigned i=0;i<servicesNode.size();i++) {
                Service service;
                servicesNode[i] >> service;

				if (service.id == 0)
					config.proxys.push_back(service);
				else
                	config.services.push_back(service);
            }
        }
        catch(int){
            cerr << "parse yaml error, service is empty" << endl;
            exit(-1);
        }
    }
}

int Megatron::init(const std::string& instanceName)
{
    instanceName_ = instanceName;
    config_.global.monitor = true;
    config_.global.realtime = true;
    return ExtractCpuNum();
}

int Megatron::parseYAML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName)
{
    try{
        std::ifstream fin(sppYAMLFileName.c_str(), std::ifstream::in);
        if(!fin.is_open())
        {
            cout << "[DEBUG]" << sppYAMLFileName << " not exist." << endl;
        }
        else
        {
            YAML::Parser parser(fin);

            YAML::Node doc;
            parser.GetNextDocument(doc);
            doc >> config_;
        }
        fin.close();

    } catch(YAML::ParserException& e) {
        cerr << sppYAMLFileName << " parse exception:"
            << e.what() << endl;
        return -2;
    }
    try{
        std::ifstream fin(serviceYAMLFileName.c_str(), std::ifstream::in);
        if(!fin.is_open())
        {
            cerr << "open " << serviceYAMLFileName << " error." << endl;
            fin.close();
            return -1;
        }

        YAML::Parser parser(fin);

        YAML::Node doc;
        parser.GetNextDocument(doc);
        doc >> config_;
        fin.close();
    } catch(YAML::ParserException& e) {
        cerr << serviceYAMLFileName << " parse exception:"
            << e.what() << endl;
        return -2;
    }

    return 0;
}

int Megatron::parseXML(std::string& XMLFileName)
{
    TiXmlDocument docCtrl( XMLFileName.c_str() );
    if(!docCtrl.LoadFile())
    {
        cout << "load xml error" << endl;
        return -1;
    }

    TiXmlHandle handleCtrl(&docCtrl);
    // <controller>
    TiXmlElement* controllerElement = handleCtrl.FirstChildElement().Element();
    // should always have a valid root but handle gracefully if it does
    if (!controllerElement)
    {
        cout << "xml format error: <controller> not found" << endl;
        return -1;
    }

    TiXmlHandle handleController = TiXmlHandle(controllerElement);
    // <procmon>
    TiXmlElement* procmonElement = handleController.FirstChildElement().Element();
    if(!procmonElement)
    {
        cout << "parse " << XMLFileName << " error: <procmon> not found" << endl;
        return -1;
    }
    TiXmlHandle handleProcmon = TiXmlHandle(procmonElement);

    TiXmlElement* groupElement = handleProcmon.FirstChild().Element();
    if(NULL == groupElement)
    {
        cout << "parse " << XMLFileName << " error: empty groups" << endl;
        return -1;
    }

    TiXmlElement* reportElement = handleController.FirstChild("report").Element();
    if(reportElement)
    {
        config_.global.report = true;
    }

    TiXmlElement* exceptionElement = handleController.FirstChild("exception").Element();
    if(exceptionElement)
    {
        int coredumpval = 1;
		int restartval = 1;
		int monitorval = 1;
        int realtimeval = 1;
    
        exceptionElement->QueryIntAttribute("coredump", &coredumpval);		
        exceptionElement->QueryIntAttribute("restart", &restartval);		
        exceptionElement->QueryIntAttribute("monitor", &monitorval);
        exceptionElement->QueryIntAttribute("realtime", &realtimeval);
        config_.global.coredump = (bool)coredumpval;
        config_.global.restart = (bool)restartval;
        config_.global.monitor = (bool)monitorval;
        config_.global.realtime = (bool)realtimeval;
    }
    else
    {
        //
    }

    bool isWorkerExist = false;
    for(; groupElement; groupElement = groupElement->NextSiblingElement())
    {
        int id;
        groupElement->QueryIntAttribute("id", &id);
        cout << "parse group " << id << endl;
        if(id == 0)
        {
            // parse spp_proxy.xml
            const char* proxyXMLFileName = groupElement->Attribute("etc");
            TiXmlDocument docProxy(proxyXMLFileName);
            if(!docProxy.LoadFile())
            {
                cout << "load " << proxyXMLFileName << " error.["
                    << docProxy.ErrorRow() << "]"
                    << docProxy.ErrorDesc() << endl;
                return -1;
            }

            TiXmlHandle handleRoot(&docProxy);
            // <proxy>
            TiXmlElement* proxyElement = handleRoot.FirstChild("proxy").Element();
            // should always have a valid root but handle gracefully if it does
            if (!proxyElement)
            {
                cout << "xml format error: <proxy> not found" << endl;
                return -1;
            }

            TiXmlHandle handleProxy = TiXmlHandle(proxyElement);
            // <acceptor>
            TiXmlElement* acceptorElement = handleProxy.FirstChild("acceptor").Element();
            if(!acceptorElement)
            {
                cout << "parse proxy xml error: <acceptor> not found" << endl;
                return -1;
            }
            acceptorElement->QueryIntAttribute("timeout", &config_.global.timeout);
            acceptorElement->QueryIntAttribute("maxconn", &config_.global.maxconn);

            TiXmlHandle handleAcceptor = TiXmlHandle(acceptorElement);
            // <entry>
            TiXmlElement* entryElement = handleAcceptor.FirstChild("entry").Element();
            if(NULL == entryElement)
            {
                cout << "parse proxy xml error: no bindings" << endl;
                return -1;
            }
            for(; entryElement; entryElement = entryElement->NextSiblingElement())
            {
                Listen listen;
                std::string type;
                std::string ifStr;
                int port;
                int oob = 0;
                int abstract = 1;
                const char* ptype = entryElement->Attribute("type");
                type = ptype;
                if(type == "unix")
                {
                    const char* path = entryElement->Attribute("path");

                    stringstream ss;
                    if(entryElement->QueryIntAttribute("abstract", &abstract) == TIXML_NO_ATTRIBUTE)
                        listen.address = path;
                    else {
                        ss << path;
                        if(abstract == 0)
                            ss << "-abstract";

                        listen.address = ss.str();
                    }
                }
                else // tcp / udp
                {
                    const char* pif = entryElement->Attribute("if");
                    ifStr = pif;
                    entryElement->QueryIntAttribute("port", &port);
                    stringstream ss;
                    if(entryElement->QueryIntAttribute("oob", &oob) == TIXML_NO_ATTRIBUTE)
                        ss << ifStr << ":" << port << "/" << type;
                    else {
                        if(oob == 0)
                            ss << ifStr << ":" << port << "/" << type;
                        else
                            ss << ifStr << ":" << port << "/" << type << "-oob";
                    }

                    listen.address = ss.str();

                    entryElement->QueryIntAttribute("TOS", &config_.global.TOS);
                }
                config_.global.listens.push_back(listen);
            }
            // white black list
            TiXmlElement* listElement = handleProxy.FirstChild("iptable").Element();
            if (listElement != NULL)
            {
                listElement->QueryStringAttribute("whitelist", &config_.global.whitelist);
                listElement->QueryStringAttribute("blacklist", &config_.global.blacklist);
            }

            TiXmlElement* limitElement = handleProxy.FirstChild("limit").Element();
            if(limitElement)
            {
                limitElement->QueryIntAttribute("sendcache", (int*)&config_.global.sendcache_limit);
            }

            cout << "proxy xml parsed OK." << endl;
        } else {
            Service service;
            service.id = id;
            const char* workerXMLFileName = groupElement->Attribute("etc");
            cout << "worker xml file name:" << workerXMLFileName << endl;
            groupElement->QueryIntAttribute("maxprocnum", &service.procnum);
            groupElement->QueryIntAttribute("heartbeat", &service.heartbeat);
            groupElement->QueryIntAttribute("reload", &service.reload);

            // parse spp_workerN.xml
            TiXmlDocument docWorker(workerXMLFileName);
            if(!docWorker.LoadFile())
            {
                cout << "load " << workerXMLFileName << " error" << endl;
                return -1;
            }

            TiXmlHandle handleRoot(&docWorker);
            // <worker>
            TiXmlElement* workerElement = handleRoot.FirstChildElement().Element();
            // should always have a valid root but handle gracefully if it does
            if (!workerElement)
            {
                cout << "xml format error: <worker> not found" << endl;
                return -1;
            }
            
            workerElement->QueryIntAttribute("L5us", &config_.global.L5us);
            workerElement->QueryIntAttribute("shm_fifo", &config_.global.shm_fifo);
            workerElement->QueryUnsignedAttribute("link_timeout_limit", &config_.global.link_timeout_limit);

            TiXmlHandle handleWorker = TiXmlHandle(workerElement);

            TiXmlElement* acceptorElement = handleWorker.FirstChild("acceptor").Element();
            TiXmlHandle handleAcceptor = TiXmlHandle(acceptorElement);
            // <entry>
            TiXmlElement* entryElement = handleAcceptor.FirstChild("entry").Element();
            if(entryElement)
            {
                entryElement->QueryIntAttribute("recv_size", &service.shmsize);
                entryElement->QueryIntAttribute("msg_timeout", &service.timeout);
                entryElement->QueryStringAttribute("type", &service.type);
            }

            // <flog>
            TiXmlElement* flogElement = handleWorker.FirstChild("flog").Element();
            if(flogElement)
            {
                flogElement->QueryIntAttribute("level", &service.flog.level);
                flogElement->QueryIntAttribute("type", &service.flog.type);
                flogElement->QueryIntAttribute("maxfilesize", &service.flog.maxfilesize);
                flogElement->QueryIntAttribute("maxfilenum", &service.flog.maxfilenum);
            }
            else
            {
                // cout << "[DEBUG]parse <flog> not found." << endl;
            }
            // <log>
            TiXmlElement* logElement = handleWorker.FirstChild("log").Element();
            if(logElement)
            {
                logElement->QueryIntAttribute("level", &service.log.level);
                logElement->QueryIntAttribute("type", &service.log.type);
                logElement->QueryIntAttribute("maxfilesize", &service.log.maxfilesize);
                logElement->QueryIntAttribute("maxfilenum", &service.log.maxfilenum);
            }
            else
            {
                // cout << "[DEBUG]parse <log> not found." << endl;
            }

            TiXmlElement* exceptionElement = handleWorker.FirstChild("exception").Element();
            if(exceptionElement)
            {
                int packetdumpval = 0;
            
                exceptionElement->QueryIntAttribute("packetdump", &packetdumpval);
                service.packetdump      = (bool)packetdumpval;
            }
            else
            {
                //
            }

            // <module>
            TiXmlElement* moduleElement = handleWorker.FirstChild("module").Element();
            if(!moduleElement)
            {
                cout << "parse worker xml error: <module> not found" << endl;
                return -1;
            }
            const char* pmodule = moduleElement->Attribute("bin");
            service.module = pmodule;
            const char* pconf = moduleElement->Attribute("etc");
            service.conf = pconf;
            moduleElement->QueryIntAttribute("global", &service.global);
            config_.services.push_back(service);
            isWorkerExist = true;
            cout << "worker " << id << " xml parsed OK." << endl;
        }
    }

    if(!isWorkerExist)
    {
        cerr << "worker group not found!" << endl;
        return -1;
    }

    return 0;
}

int Megatron::writeYAMLglobal(YAML::Emitter& out)
{
    out << YAML::Key << "global" << YAML::Value
        << YAML::BeginMap // begin listen
        << YAML::Key << "listen" << YAML::Value
        << YAML::BeginSeq;
    std::vector<Listen>::iterator listenIt;
    for(listenIt = config_.global.listens.begin();
            listenIt!= config_.global.listens.end();
            listenIt++)
    {
        out << YAML::BeginMap // begin address
            << YAML::Key << "address" << YAML::Value << listenIt->address
            << YAML::EndMap; // end address
    }
    out << YAML::EndSeq; // end listen

    if (config_.global.sendcache_limit)
    {
        out << YAML::Key << "limit" << YAML::Value
            << YAML::BeginMap
            << YAML::Key << "sendcache" << YAML::Value << config_.global.sendcache_limit
            << YAML::EndMap;
    }

    if(-1 != config_.global.timeout)
    {
        out << YAML::Key << "timeout" << YAML::Value << config_.global.timeout;
    }
    if(-1 != config_.global.TOS)
    {
        out << YAML::Key << "TOS" << YAML::Value << config_.global.TOS;
    }
    if(-1 != config_.global.L5us)
    {
        out << YAML::Key << "L5us" << YAML::Value << config_.global.L5us;
    }    

    if(-1 != config_.global.shm_fifo)
    {
        out << YAML::Key << "shm_fifo" << YAML::Value << config_.global.shm_fifo;
    }  

    if (config_.global.link_timeout_limit)
    {
        out << YAML::Key << "link_timeout_limit" << YAML::Value << config_.global.link_timeout_limit;
    }

    if(-1 != config_.global.maxconn)
    {
        out << YAML::Key << "maxconn" << YAML::Value << config_.global.maxconn;
    }

    if (config_.global.whitelist.size() != 0)
    {
        out << YAML::Key << "whitelist" << YAML::Value << config_.global.whitelist;
    }
    if (config_.global.blacklist.size() != 0)
    {
        out << YAML::Key << "blacklist" << YAML::Value << config_.global.blacklist;
    }

    out << YAML::EndMap; // end global
    return 0;
}

int Megatron::writeYAMLservices(YAML::Emitter& out)
{
    out << YAML::Key << "service" << YAML::Value
        << YAML::BeginSeq; // begin services

    std::vector<Service>::iterator serviceIt;
    for(serviceIt = config_.services.begin();
            serviceIt != config_.services.end();
            serviceIt++)
    {
        out << YAML::BeginMap // begin service
            << YAML::Key << "id" << YAML::Value << serviceIt->id
            << YAML::Key << "module" << YAML::Value << serviceIt->module
            << YAML::Key << "procnum" << YAML::Value << serviceIt->procnum;
        if (serviceIt->global)
        {
            out << YAML::Key << "global" << YAML::Value << serviceIt->global;
        }
        if(-1 != serviceIt->heartbeat)
        {
            out << YAML::Key << "heartbeat" << YAML::Value << serviceIt->heartbeat;
        }

        //davisxie
        if (true == serviceIt->packetdump)
        {
            out << addYAMLKeyValue("packetdump", "on");
        }
        
        if(!serviceIt->conf.empty())
        {
            out << YAML::Key << "conf" << YAML::Value << serviceIt->conf;
        }
        if(!serviceIt->type.empty())
        {
            out << YAML::Key << "type" << YAML::Value << serviceIt->type;
        }
        if(!serviceIt->flog.empty())
        {
            out << YAML::Key << "flog" << YAML::Value
                << YAML::BeginMap;
            if(-1 != serviceIt->flog.level)
            {
                out << YAML::Key << "level" << YAML::Value << serviceIt->flog.level;
            }
            if(-1 != serviceIt->flog.type)
            {
                out << YAML::Key << "type" << YAML::Value << serviceIt->flog.type;
            }
            if(-1 != serviceIt->flog.maxfilesize)
            {
                out << YAML::Key << "maxfilesize" << YAML::Value << serviceIt->flog.maxfilesize;
            }
            if(-1 != serviceIt->flog.maxfilenum)
            {
                out << YAML::Key << "maxfilenum" << YAML::Value << serviceIt->flog.maxfilenum;
            }
            out << YAML::EndMap;
        }
        else
        {
            // cout << "[DEBUG]flog not found." << endl;
        }
        if(!serviceIt->log.empty())
        {
            out << YAML::Key << "log" << YAML::Value
                << YAML::BeginMap;
            if(-1 != serviceIt->log.level)
            {
                out << YAML::Key << "level" << YAML::Value << serviceIt->log.level;
            }
            if(-1 != serviceIt->log.type)
            {
                out << YAML::Key << "type" << YAML::Value << serviceIt->log.type;
            }
            if(-1 != serviceIt->log.maxfilesize)
            {
                out << YAML::Key << "maxfilesize" << YAML::Value << serviceIt->log.maxfilesize;
            }
            if(-1 != serviceIt->log.maxfilenum)
            {
                out << YAML::Key << "maxfilenum" << YAML::Value << serviceIt->log.maxfilenum;
            }
            out << YAML::EndMap;
        }
        if(-1 != serviceIt->shmsize)
        {
            out << YAML::Key << "shmsize" << YAML::Value << serviceIt->shmsize;
        }
        if(-1 != serviceIt->timeout)
        {
            out << YAML::Key << "timeout" << YAML::Value << serviceIt->timeout;
        }
        out << YAML::EndMap; // end service
    }

    out << YAML::EndSeq; // end services
    return 0;
}

int Megatron::writeYAML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName)
{
    if(config_.global.report == true)
    {
        try{
            std::ofstream fout(sppYAMLFileName.c_str(), std::ifstream::out);
            YAML::Emitter out;

        out << YAML::BeginMap
                << YAML::Key << "global" << YAML::Value
                << YAML::BeginMap
                    << addYAMLKeyValue("type", "spp");

        if (config_.global.report == true)
            out << addYAMLKeyValue("report", "on");
        if (config_.global.coredump == true)
            out << addYAMLKeyValue("coredump", "on");
        if (config_.global.restart == true)
            out << addYAMLKeyValue("restart", "on");
        if (config_.global.monitor == true)
            out << addYAMLKeyValue("monitor", "on");
        if (config_.global.realtime == true)
            out << addYAMLKeyValue("realtime","on");

           out << YAML::EndMap // end global
            << YAML::EndMap; // end file

        fout << out.c_str();
        fout.flush();

        } catch(YAML::ParserException& e) {
            cerr << "write " << sppYAMLFileName << " exception: "
                << e.what() << endl;
            return -1;
        }
    }

    try{
        std::ofstream fout(serviceYAMLFileName.c_str(), std::ifstream::out);
        YAML::Emitter out;

        out << YAML::BeginMap;
        writeYAMLglobal(out);
        writeYAMLservices(out);
        out << YAML::EndMap;

        fout << out.c_str();
        fout.flush();

    } catch(YAML::ParserException& e) {
        cerr << "write " << serviceYAMLFileName << " exception: "
            << e.what() << endl;
        return -1;
    }

    return 0;
}

int Megatron::writeXML(std::string& XMLFileName)
{
    int ret = 0;
    std::vector<Service>::iterator serviceIt;
    std::vector<Listen>::iterator listenIt;
    std::string proxyExeName = instanceName_ + "_proxy";
    std::string workerExeName = instanceName_ + "_worker";

    //pre check
    if(0 == config_.services.size())
    {
        cerr << "[ERROR]No services in yaml." << endl;
        return -1;
    }

    // write spp_ctrl.xml
    TiXmlDocument docCtrl;
    TiXmlDeclaration * declCtrl = new TiXmlDeclaration( "1.0", "utf-8", "" );
    docCtrl.LinkEndChild( declCtrl );

    TiXmlElement* controllerElement = new TiXmlElement( "controller" );
    docCtrl.LinkEndChild( controllerElement );
    
    TiXmlElement* procmonElement = new TiXmlElement( "procmon" );
    controllerElement->LinkEndChild( procmonElement );

    TiXmlElement* proxyGroupElement = new TiXmlElement("group" );
    proxyGroupElement->SetAttribute("id", 0);
    proxyGroupElement->SetAttribute("exe", proxyExeName.c_str());
    proxyGroupElement->SetAttribute("etc", "../etc/spp_proxy.xml");
    proxyGroupElement->SetAttribute("maxprocnum", 1);
    proxyGroupElement->SetAttribute("minprocnum", 1);
    procmonElement->LinkEndChild( proxyGroupElement );

    for(serviceIt = config_.services.begin(); serviceIt != config_.services.end(); serviceIt++)
    {
        TiXmlElement* groupElement = new TiXmlElement("group" );

        groupElement->SetAttribute("id", serviceIt->id);
        groupElement->SetAttribute("exe", workerExeName.c_str());
        stringstream ss;
        ss << "../etc/spp_worker" << serviceIt->id << ".xml";
        groupElement->SetAttribute("etc", ss.str().c_str());
        groupElement->SetAttribute("maxprocnum", serviceIt->procnum);
        groupElement->SetAttribute("minprocnum", serviceIt->procnum);
        if(-1 != serviceIt->heartbeat)
            groupElement->SetAttribute("heartbeat", serviceIt->heartbeat);
        if(0 != serviceIt->reload)
            groupElement->SetAttribute("reload", serviceIt->reload);

        procmonElement->LinkEndChild( groupElement );
    }
    if(config_.global.report == true)
    {
        stringstream reportss;
        reportss << "./" << instanceName_ << "_dc_tool";
        TiXmlElement* reportElement = new TiXmlElement("report");
        reportElement->SetAttribute("exe", reportss.str().c_str());
        controllerElement->LinkEndChild( reportElement );
    }

    if (true == config_.global.coredump 
        || true == config_.global.restart 
        || true == config_.global.monitor
        || true == config_.global.realtime)
    {
        TiXmlElement* exceptionElement = new TiXmlElement("exception");

		if (true == config_.global.coredump)
	        exceptionElement->SetAttribute("coredump", (int)config_.global.coredump);

		if (true == config_.global.restart)
	        exceptionElement->SetAttribute("restart", (int)config_.global.restart);

		if (true == config_.global.monitor)
	        exceptionElement->SetAttribute("monitor", (int)config_.global.monitor);

		if (true == config_.global.realtime)
       	  exceptionElement->SetAttribute("realtime", (int)config_.global.realtime);

		controllerElement->LinkEndChild( exceptionElement );
    }
    
    // write spp_proxy.xml
    TiXmlDocument docProxy;
    TiXmlDeclaration * declProxy= new TiXmlDeclaration( "1.0", "utf-8", "" );
    docProxy.LinkEndChild( declProxy);

    TiXmlElement* proxyElement = new TiXmlElement( "proxy" );


    if (-1 != config_.global.shm_fifo)
    {
        proxyElement->SetAttribute("shm_fifo", config_.global.shm_fifo);
    }

    docProxy.LinkEndChild( proxyElement );


    TiXmlElement* acceptorElement = new TiXmlElement( "acceptor" );
    if(-1 != config_.global.timeout)
    {
        acceptorElement->SetAttribute("timeout", config_.global.timeout);
    }
    if(true != config_.global.udpclose)
    {
        acceptorElement->SetAttribute("udpclose", config_.global.udpclose);
    }
    if(-1 != config_.global.maxconn)
    {
        acceptorElement->SetAttribute("maxconn", config_.global.maxconn);
    }
    proxyElement->LinkEndChild( acceptorElement );

    if (config_.global.sendcache_limit)
    {
        TiXmlElement* limitElement = new TiXmlElement( "limit" );
        limitElement->SetAttribute("sendcache", config_.global.sendcache_limit);
        proxyElement->LinkEndChild(limitElement);
    }

    for(listenIt = config_.global.listens.begin(); listenIt != config_.global.listens.end(); listenIt++)
    {
        if(listenIt->address.empty())
        {
            continue;
        }

        TiXmlElement* entryElement = new TiXmlElement( "entry" );

        if(listenIt->address[0] == '/') // unix path
        {
            std::string path;
            int abstract;

            entryElement->SetAttribute("type", "unix");
            ret = parseUnixAddress(listenIt->address, path, abstract);

            entryElement->SetAttribute("path", path.c_str());
            if(abstract == 0)
                entryElement->SetAttribute("abstract", abstract);
        }
        else
        {
            std::string type;
            std::string ifStr;
            int port;
            int oob;
            ret = parseAddress(listenIt->address, type, ifStr, port, oob);
            if(ret < 0)
            {
                return ret;
            }
            entryElement->SetAttribute("type", type.c_str());
            entryElement->SetAttribute("if", ifStr.c_str());
            entryElement->SetAttribute("port", port);
            if(oob == 1)
                entryElement->SetAttribute("oob", oob);
        }
        if(-1 != config_.global.TOS)
        {
            entryElement->SetAttribute("TOS", config_.global.TOS);
        }
        acceptorElement->LinkEndChild(entryElement);
    }

    TiXmlElement* connectorElement = new TiXmlElement( "connector");
    proxyElement->LinkEndChild( connectorElement );

    for(serviceIt = config_.services.begin(); serviceIt != config_.services.end(); serviceIt++)
    {
        TiXmlElement* entryElement = new TiXmlElement( "entry" );
        entryElement->SetAttribute("groupid", serviceIt->id);
        if(-1 != serviceIt->shmsize)
        {
            entryElement->SetAttribute("send_size", serviceIt->shmsize);
            entryElement->SetAttribute("recv_size", serviceIt->shmsize);
        }
        if (!serviceIt->type.empty())
        {
            entryElement->SetAttribute("type", serviceIt->type.c_str());
        }
        connectorElement->LinkEndChild( entryElement );
    }

    // write proxy <flog> and <log> according to worker 1
	if (config_.proxys.size() > 0)
		serviceIt = config_.proxys.begin(); 
	else
    	serviceIt = config_.services.begin();

    if(!serviceIt->flog.empty())
    {
        TiXmlElement* flogElement = new TiXmlElement( "flog" );
        if(-1 != serviceIt->flog.level)
        {
            flogElement->SetAttribute("level", serviceIt->flog.level);
        }
        if(-1 != serviceIt->flog.type)
        {
            flogElement->SetAttribute("type", serviceIt->flog.type);
        }
        if(-1 != serviceIt->flog.maxfilesize)
        {
            flogElement->SetAttribute("maxfilesize", serviceIt->flog.maxfilesize);
        }
        if(-1 != serviceIt->flog.maxfilenum)
        {
            flogElement->SetAttribute("maxfilenum", serviceIt->flog.maxfilenum);
        }
        proxyElement->LinkEndChild( flogElement );
    }

    if(!serviceIt->log.empty())
    {
        TiXmlElement* logElement = new TiXmlElement( "log" );
        if(-1 != serviceIt->log.level)
        {
            logElement->SetAttribute("level", serviceIt->log.level);
        }
        if(-1 != serviceIt->log.type)
        {
            logElement->SetAttribute("type", serviceIt->log.type);
        }
        if(-1 != serviceIt->log.maxfilesize)
        {
            logElement->SetAttribute("maxfilesize", serviceIt->log.maxfilesize);
        }
        if(-1 != serviceIt->log.maxfilenum)
        {
            logElement->SetAttribute("maxfilenum", serviceIt->log.maxfilenum);
        }
        proxyElement->LinkEndChild( logElement );
    }

    TiXmlElement* moduleElement = new TiXmlElement( "module" );
    // proxy module == worker 1 module
    moduleElement->SetAttribute("bin", serviceIt->module.c_str());
    moduleElement->SetAttribute("etc", serviceIt->conf.c_str());
    if (serviceIt->global)
        moduleElement->SetAttribute("global", serviceIt->global);
    proxyElement->LinkEndChild( moduleElement );

    TiXmlElement* resultElement = new TiXmlElement( "result" );
    // proxy module == worker 1 module
    resultElement->SetAttribute("bin", serviceIt->result.c_str());
    proxyElement->LinkEndChild( resultElement );
	
    if (config_.global.whitelist != "" || config_.global.blacklist != "")
    {
        TiXmlElement* listElement = new TiXmlElement("iptable");
        if (config_.global.whitelist != "")
        {
            listElement->SetAttribute("whitelist", config_.global.whitelist);
        }
        if (config_.global.blacklist != "")
        {
            listElement->SetAttribute("blacklist", config_.global.blacklist);
        }
        proxyElement->LinkEndChild(listElement);
    }

	if (true == config_.global.monitor || true == config_.global.realtime)
	{	
    	TiXmlElement* exceptionElement = new TiXmlElement( "exception" );
        exceptionElement->SetAttribute("monitor", (int)config_.global.monitor);		
        exceptionElement->SetAttribute("realtime", (int)config_.global.realtime);    
		proxyElement->LinkEndChild(exceptionElement);
	}

    // flush to files
    docCtrl.SaveFile( XMLFileName.c_str() );
    docProxy.SaveFile( "../etc/spp_proxy.xml" );

    // write spp_workerN.xml
    for(serviceIt = config_.services.begin(); serviceIt != config_.services.end(); serviceIt++)
    {
        TiXmlDocument docWorker;
        TiXmlDeclaration * declWorker = new TiXmlDeclaration( "1.0", "utf-8", "" );
        docWorker.LinkEndChild( declWorker);

        TiXmlElement* workerElement = new TiXmlElement( "worker" );
        workerElement->SetAttribute("groupid", serviceIt->id);
        if(-1 != config_.global.TOS)
        {
            workerElement->SetAttribute("TOS", config_.global.TOS);
        }
        if (-1 != config_.global.L5us)
        {
            workerElement->SetAttribute("L5us", config_.global.L5us);
        }
        if (-1 != config_.global.shm_fifo)
        {
            workerElement->SetAttribute("shm_fifo", config_.global.shm_fifo);
        }
        if (config_.global.link_timeout_limit)
        {
            workerElement->SetAttribute("link_timeout_limit", config_.global.link_timeout_limit);
        }
        if (!config_.global.proxy_ip.empty())
        {
            workerElement->SetAttribute("proxy", config_.global.proxy_ip);
        }
        docWorker.LinkEndChild( workerElement );

        TiXmlElement* acceptorElement = new TiXmlElement( "acceptor" );
        workerElement->LinkEndChild( acceptorElement );

        TiXmlElement* entryElement = new TiXmlElement( "entry" );
        if(-1 != serviceIt->shmsize)
        {
            entryElement->SetAttribute("send_size", serviceIt->shmsize);
            entryElement->SetAttribute("recv_size", serviceIt->shmsize);
        }
        if(-1 != serviceIt->timeout)
        {
            entryElement->SetAttribute("msg_timeout", serviceIt->timeout);
        }
        if (!serviceIt->type.empty())
        {
            entryElement->SetAttribute("type", serviceIt->type.c_str());
        }        
        acceptorElement->LinkEndChild( entryElement );

        if(!serviceIt->flog.empty())
        {
            // cout << "[DEBUG]write worker xml <flog>" << endl;
            TiXmlElement* flogElement = new TiXmlElement( "flog" );
            if(-1 != serviceIt->flog.level)
            {
                // cout << "[DEBUG]write worker xml <flog> level" << endl;
                flogElement->SetAttribute("level", serviceIt->flog.level);
            }
            if(-1 != serviceIt->flog.type)
            {
                flogElement->SetAttribute("type", serviceIt->flog.type);
            }
            if(-1 != serviceIt->flog.maxfilesize)
            {
                flogElement->SetAttribute("maxfilesize", serviceIt->flog.maxfilesize);
            }
            if(-1 != serviceIt->flog.maxfilenum)
            {
                flogElement->SetAttribute("maxfilenum", serviceIt->flog.maxfilenum);
            }
            workerElement->LinkEndChild( flogElement );
        }

        if(!serviceIt->log.empty())
        {
            // cout << "[DEBUG]write worker xml <log>" << endl;
            TiXmlElement* logElement = new TiXmlElement( "log" );
            if(-1 != serviceIt->log.level)
            {
                // cout << "[DEBUG]write worker xml <log> level" << endl;
                logElement->SetAttribute("level", serviceIt->log.level);
            }
            if(-1 != serviceIt->log.type)
            {
                logElement->SetAttribute("type", serviceIt->log.type);
            }
            if(-1 != serviceIt->log.maxfilesize)
            {
                logElement->SetAttribute("maxfilesize", serviceIt->log.maxfilesize);
            }
            if(-1 != serviceIt->log.maxfilenum)
            {
                logElement->SetAttribute("maxfilenum", serviceIt->log.maxfilenum);
            }
            workerElement->LinkEndChild( logElement );
        }


		//if (true == config_.global.monitor || true == serviceIt->packetdump || true == config_.global.restart)
        {
            TiXmlElement* exceptionElement = new TiXmlElement("exception");

            exceptionElement->SetAttribute("packetdump", (int)serviceIt->packetdump);
            exceptionElement->SetAttribute("monitor", (int)config_.global.monitor);
            exceptionElement->SetAttribute("restart", (int)config_.global.restart);
            exceptionElement->SetAttribute("realtime", (int)config_.global.realtime);
            workerElement->LinkEndChild( exceptionElement );
        }

        TiXmlElement* moduleElement = new TiXmlElement( "module" );
        moduleElement->SetAttribute("bin", serviceIt->module.c_str());
        moduleElement->SetAttribute("etc", serviceIt->conf.c_str());
        if (serviceIt->global)
            moduleElement->SetAttribute("global", serviceIt->global);
        workerElement->LinkEndChild( moduleElement );

        TiXmlElement* sessionElement = new TiXmlElement( "session_config" );
        sessionElement->SetAttribute("etc", "");
        sessionElement->SetAttribute("max_epoll_timeout", "10");
        workerElement->LinkEndChild( sessionElement );

        stringstream ss;
        ss << "../etc/spp_worker" << serviceIt->id << ".xml";
        docWorker.SaveFile( ss.str().c_str() );
    }

    return 0;
}

int Megatron::parseAddress(std::string& address, std::string& type, std::string& ifStr, int& port, int& oob)
{
    size_t colonPos = address.rfind(':');
    if(colonPos == string::npos)
    {
        cout << "address: " << address << " : not found" << endl;
        return -1;
    }
    ifStr  = address.substr(0, colonPos);

    size_t slashPos = address.find('/');
    if(slashPos == string::npos)
    {
        cout << "address: " << address << " / not found" << endl;
        return -1;
    }
    port = atoi(address.substr(colonPos + 1, slashPos-colonPos-1).c_str());

    size_t shortPos = address.find('-');
    if(shortPos == string::npos)
    {
        oob = 0;
        type = address.substr(slashPos + 1);
    }
    else {
        type = address.substr(slashPos + 1, shortPos-slashPos-1);
        oob = 0;
        if(address.substr(shortPos + 1) == "oob")
            oob = 1;
    }

    return 0;
}

int Megatron::parseUnixAddress(std::string& address, std::string& path, int& abstract)
{
    size_t colonPos = address.rfind('-');
    if(colonPos == string::npos)
    {
        cout << "address: " << address << " - not found" << endl;
        path = address;
        abstract = 1;
        return 0;
    }

    //yaml中有配置abstract
    path = address.substr(0, colonPos);
    if(address.substr(colonPos + 1) == "abstract")
        abstract = 0;
    else
        abstract = 1;

    return 0;
}


int Megatron::YAMLtoXML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName, std::string& XMLFileName)
{
    int ret = 0;

    if((ret = parseYAML(sppYAMLFileName, serviceYAMLFileName)) < 0)
    {
        return ret;
    }

    if((ret = writeXML(XMLFileName)) < 0)
    {
        return ret;
    }

    return 0;
}

int Megatron::XMLtoYAML(std::string& XMLFileName, std::string& sppYAMLFileName, std::string& serviceYAMLFileName)
{
    int ret = 0;
    if((ret = parseXML(XMLFileName)) < 0)
    {
        cout << "parse xml file " << XMLFileName << " failed." << endl;
        return ret;
    }
    if((ret = writeYAML(sppYAMLFileName, serviceYAMLFileName)) < 0)
    {
        cout << "write yaml file " << serviceYAMLFileName << " failed." << endl;
        return ret;
    }

    return 0;
}

int Megatron::dumpYAML(std::string& outStr)
{
    return 0;
}

int Megatron::dumpXML(std::string& outStr)
{
    return 0;
}
