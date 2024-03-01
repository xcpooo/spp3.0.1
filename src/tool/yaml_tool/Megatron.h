#ifndef __MEGATRON_H__
#define __MEGATRON_H__

#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"

struct Listen
{
    std::string address;
};

struct Global
{
    bool report;
    bool coredump;
	bool restart;
	bool monitor;
    std::vector<Listen> listens;
    int timeout;
    bool udpclose;
    int TOS;
    int oob;
    int maxconn;
    std::string whitelist;
    std::string blacklist;
    int L5us; // L5 global us switch
    int shm_fifo;
    std::string proxy_ip; 
    std::string sub_type;
    unsigned sendcache_limit;
    unsigned link_timeout_limit;  // worker parallel link reuse limit
    bool realtime;

    Global():report(false),coredump(false),restart(true),monitor(false),timeout(-1),udpclose(true),TOS(-1),maxconn(-1),L5us(-1),shm_fifo(-1),sub_type("spp"), sendcache_limit(0),link_timeout_limit(0),realtime(true){}
};

struct Log
{
    int level;
    int type;
    int maxfilesize;
    int maxfilenum;

    Log():level(-1),type(-1),maxfilesize(-1),maxfilenum(-1){}

    bool empty()
    {
        return (level == -1
            && type == -1
            && maxfilesize == -1
            && maxfilenum == -1);
    }
};

struct Service
{
    int id;
    std::string module;
	std::string result;
    std::string conf;
    std::string type;
    int procnum;
    Log flog;
    Log log;
    int heartbeat;
    int reload;
    int global;  // dlopen flag RTLD_GLOBAL
    int shmsize; // unit:MB
    int timeout; // unit:ms
    bool packetdump;

    Service():heartbeat(-1), reload(0), global(0), shmsize(-1), timeout(-1),packetdump(false){}
};


struct Config
{
    Global global;	
    std::vector<Service> proxys;
    std::vector<Service> services;
};

class Megatron
{
public:
    Megatron(){};
    ~Megatron(){};
    int init(const std::string& instanceName);

    int YAMLtoXML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName, std::string& XMLFileName);
    int XMLtoYAML(std::string& XMLFileName, std::string& sppYAMLFileName, std::string& serviceYAMLFileName);

    int dumpYAML(std::string& outStr);
    int dumpXML(std::string& outStr);
private:
    int parseYAML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName);
    int parseXML(std::string& XMLFileName);
    int writeYAML(std::string& sppYAMLFileName, std::string& serviceYAMLFileName);
    int writeXML(std::string& XMLFileName);
    int parseAddress(std::string& address, std::string& type, std::string& ifStr, int& port, int& oob);
    int parseUnixAddress(std::string& address, std::string& path, int& abstract);
    int writeYAMLglobal(YAML::Emitter& out);
    int writeYAMLservices(YAML::Emitter& out);

private:
    Config config_;
    std::string instanceName_;
};
#endif
