#include "ICostStat.h"
#include "StatMgrInstance.h"

USING_SPP_STAT_NS;

ICostStat* ICostStat::instance()
{
    return (ICostStat*) CStatMgrInstance::instance();    
}
