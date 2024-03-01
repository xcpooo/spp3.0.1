#ifndef __STATMGR_INSTANCE_H__
#define __STATMGR_INSTANCE_H__

#include "StatMgr.h"
#include "StatComDef.h"

BEGIN_SPP_STAT_NS
class CStatMgrInstance
{
    public:
        
        static CStatMgr* instance();
        static void destroy();
    private:
        static CStatMgr* _mgr;

        CStatMgrInstance();
        ~CStatMgrInstance();
};
END_SPP_STAT_NS
#endif
