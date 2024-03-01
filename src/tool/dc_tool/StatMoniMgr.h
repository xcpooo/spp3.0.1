#ifndef __STATMONI_MGR_H__
#define __STATMONI_MGR_H__
#include <string>
#include <map>
#include "StatComDef.h"
BEGIN_SPP_STAT_NS
typedef struct
{
    unsigned group;
    unsigned cmdid;
    unsigned code;
    unsigned count;
    unsigned timecost;
    unsigned timemap[MAX_TIMEMAP_SIZE];

}moni_code_data_t;
class CStatMoniMgr
{
    typedef struct
    {
        StatMgrPool* _mappool;
        unsigned     _groupid;
        bool         _curIndex;
        StatMgrPool  _pool[2];
    }StatData;

    typedef std::map<int, StatData> StatMap;
    typedef int (*CBFunction)(void*); 
    public:
        CStatMoniMgr();
        virtual ~CStatMoniMgr();

        int init();

        int run();

        int regCallBack(CBFunction func);
        
    protected:
        int proc(const StatMgrPool& oldPool, const StatMgrPool& newPool);
        void fini();

        int initPool(StatMgrPool* &pool, const char* filepath);
    private:
        unsigned _groupid;
        StatMap _statMap;
        CBFunction _cbFuncProc;
};
END_SPP_STAT_NS
#endif
