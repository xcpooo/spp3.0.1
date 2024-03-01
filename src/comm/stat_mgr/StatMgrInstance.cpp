#include "StatMgrInstance.h"
#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif


USING_SPP_STAT_NS;

CStatMgr* CStatMgrInstance::_mgr = NULL;

CStatMgr* CStatMgrInstance::instance()
{
    if (unlikely(_mgr == NULL))
    {
        _mgr = new CStatMgr();
    }
    return _mgr;
}
void CStatMgrInstance::destroy()
{
    if (_mgr != NULL)
    {
        delete _mgr;
        _mgr = NULL;
    }
}

CStatMgrInstance::CStatMgrInstance()
{
    return ;
}

CStatMgrInstance::~CStatMgrInstance()
{
    return;
}
