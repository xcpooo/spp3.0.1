#include "IFrameStat.h"
#include "StatMgrInstance.h"

USING_SPP_STAT_NS; 

ICodeStat* ICodeStat::instance()
{
    return (ICodeStat*)CStatMgrInstance::instance();
}

IReqStat* IReqStat::instance()
{
    return (IReqStat*) CStatMgrInstance::instance();
}

