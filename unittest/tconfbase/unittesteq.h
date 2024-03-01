
#ifndef _SPP_UNITTEST_EQ_
#define _SPP_UNITTEST_EQ_ 
#define private public
#define protected public
#include "loadconfbase.h"
using namespace spp::tconfbase;
bool equalLog(const Log& left, const Log& right);
bool equalStat(const Stat& left, const Stat& right); 
bool equalProcmon(const Procmon& left, const Procmon& right);
bool equalMoni(const Moni& left, const Moni& right);
bool equalModule(const Module& left, const Module& right);
bool equalConnectShm(const ConnectShm& left, const ConnectShm& right);
bool equalAcceptorSock(const AcceptorSock& left, const AcceptorSock& right);
bool equalSessionConfig(const SessionConfig& left, const SessionConfig& right);
bool equalReport(const Report& left, const Report& right);
bool equalProxy(const Proxy& left, const Proxy& right);
bool equalWorker(const Worker& left, const Worker& right);
bool equalIptable(const Iptable& left, const Iptable& right);
bool equalAsyncSession(const AsyncSession& left, const AsyncSession& right);
#endif
