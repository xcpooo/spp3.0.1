#include "unittesteq.h"

bool equalLog(const Log& left, const Log& right)
{
    return (left.level == right.level 
         && left.type == right.type
         && left.maxfilesize == right.maxfilesize
         && left.maxfilenum == right.maxfilenum
         && left.path == right.path);
}
bool equalStat(const Stat& left, const Stat& right)
{
    return (left.file == right.file);
}
bool equalProcmon(const Procmon& left, const Procmon& right)
{
    if (left.entry.size() != right.entry.size())
        return 0;
    for (size_t i = 0;i < left.entry.size(); ++ i)
    {
        if (left.entry[i].id != right.entry[i].id
         || left.entry[i].basepath != right.entry[i].basepath
         || left.entry[i].exe != right.entry[i].exe
         || left.entry[i].etc != right.entry[i].etc
         || left.entry[i].maxprocnum != right.entry[i].maxprocnum
         || left.entry[i].minprocnum != right.entry[i].minprocnum
         || left.entry[i].heartbeat != right.entry[i].heartbeat
         || left.entry[i].affinity !=right.entry[i].affinity
         || left.entry[i].exitsignal != right.entry[i].exitsignal)
            return 0;
    }
    return 1;
}
bool equalMoni(const Moni& left, const Moni& right)
{
    return (left.intervial == right.intervial && equalLog(left.log, right.log) == true);
}
bool equalModule(const Module& left, const Module& right)
{
    return (left.bin == right.bin && 
            left.etc == right.etc &&
            left.isGlobal == right.isGlobal &&
            left.localHandleName == right.localHandleName);
}
bool equalConnectShm(const ConnectShm& left, const ConnectShm& right)
{
    if (left.entry.size() != right.entry.size())
    {
        return 0;
    }
    if (left.type != right.type || left.scriptname != right.scriptname)
    {
        return 0;
    }
    for (size_t i = 0; i < left.entry.size(); ++ i )
    {
        if (left.entry[i].id != right.entry[i].id ||
            left.entry[i].type != right.entry[i].type ||
            left.entry[i].recv_size != right.entry[i].recv_size ||
            left.entry[i].send_size != right.entry[i].send_size ||
            left.entry[i].msg_timeout != right.entry[i].msg_timeout)
        {
            return 0;
        }
    }
    return 1;
}
bool equalAcceptorSock(const AcceptorSock& left, const AcceptorSock& right)
{
    if (left.entry.size() != right.entry.size())
    {
        return 0;
    }
    if (left.type != right.type ||
            left.timeout != right.timeout ||
            left.udpclose != right.udpclose ||
            left.maxconn != right.maxconn ||
            left.maxpkg != right.maxpkg ||
            left.type != right.type)
    {
        return 0;
    }
    for (size_t i = 0; i < left.entry.size(); ++ i)
    {
        if (left.entry[i].type != right.entry[i].type ||
            left.entry[i].ip != right.entry[i].ip ||
            left.entry[i].port != right.entry[i].port ||
            left.entry[i].path != right.entry[i].path ||
            left.entry[i].TOS != right.entry[i].TOS ||
            left.entry[i].abstract != right.entry[i].abstract)
            return 0;
    }

    return 1;
}
bool equalSessionConfig(const SessionConfig& left, const SessionConfig& right)

{
    return (left.etc == right.etc && left.epollTime == right.epollTime);
}
bool equalReport(const Report& left, const Report& right)

{
    return (left.report_tool == right.report_tool);
}
bool equalProxy(const Proxy& left, const Proxy& right)

{
    return (left.groupid == right.groupid);
}
bool equalWorker(const Worker& left, const Worker& right)

{
    return (left.groupid == right.groupid && left.TOS == right.TOS);
}
bool equalIptable(const Iptable& left, const Iptable& right)

{
    return (left.whitelist == right.whitelist && left.blacklist == right.blacklist);
}
bool equalAsyncSession(const AsyncSession& left, const AsyncSession& right)  
{
    if (left.entry.size() != right.entry.size())
        return 0;
    for (size_t i = 0; i < left.entry.size(); ++ i)
    {
        if (left.entry[i].id != right.entry[i].id ||
            left.entry[i].maintype!= right.entry[i].maintype ||
            left.entry[i].subtype != right.entry[i].subtype ||
            left.entry[i].ip != right.entry[i].ip ||
            left.entry[i].port != right.entry[i].port ||
            left.entry[i].recv_count != right.entry[i].recv_count ||
            left.entry[i].timeout != right.entry[i].timeout||
            left.entry[i].multi_con_inf != right.entry[i].multi_con_inf ||
            left.entry[i].multi_con_sup != right.entry[i].multi_con_sup)
            return 0;
    }
    return 1;
}
