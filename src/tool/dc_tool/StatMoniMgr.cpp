#include "StatMoniMgr.h"
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
USING_SPP_STAT_NS;
using namespace std;

CStatMoniMgr::CStatMoniMgr()
{
    _cbFuncProc = NULL;
    return;
}
CStatMoniMgr::~CStatMoniMgr()
{
    fini();
    return;
}
int CStatMoniMgr::regCallBack(CBFunction func)
{
    _cbFuncProc = func;
    return 0;
}

int CStatMoniMgr::init()
{
    _statMap.clear();
    char buf[512];
    int ret = 0;
    for (int i = 1; i <= 256; ++ i)
    {
        snprintf(buf, sizeof(buf) - 1, SPP_STAT_MGR_FILE, i);
        if (access(buf, R_OK) == 0) {
            StatMgrPool* pool = NULL;
            ret = initPool(pool, buf);
            if (ret != 0)
            {
                printf("[group %d] read stat pool errno = %d\n", i, ret);
                return -1;
            }

            _statMap[i]._mappool = pool;
            _statMap[i]._groupid = i;
            _statMap[i]._curIndex = 0;
            _statMap[i]._pool[0] = * (_statMap[i]._mappool);
            _statMap[i]._curIndex = !_statMap[i]._curIndex;
            printf("FOUND cost stat file [%s]\n", buf);
        }
    }
    return 0;
}
void CStatMoniMgr::fini()
{
    map<int, StatData>::iterator it = _statMap.begin(); 
    for (; it != _statMap.end(); ++ it)
    {
        ::munmap((it->second)._mappool, GET_MGR_POOL_SIZE);
        (it->second)._mappool = NULL;
    }
    _statMap.clear();
    _cbFuncProc = NULL;

}
int CStatMoniMgr::run()
{
    if (_cbFuncProc == NULL)
        return -1;
    map<int, StatData>::iterator it = _statMap.begin();
    for (; it != _statMap.end(); ++ it)
    {
        StatMgrPool* pool = it->second._mappool;
        if (pool == NULL)
        {
            printf("Stat pool is null\n");
            exit(-1);
        }
        it->second._pool[it->second._curIndex] = *pool;
        _groupid = it->first;
        proc(it->second._pool[!it->second._curIndex], it->second._pool[it->second._curIndex]);
        it->second._curIndex = !it->second._curIndex;  
    }
    return 0;
}
int CStatMoniMgr::proc(const StatMgrPool& oldPool, const StatMgrPool& newPool)
{
    moni_code_data_t data;
    data.group = _groupid;
    for (int i = 0; i < MAX_CMD_NUM; ++ i)
    {
        data.cmdid = i;
        if (newPool._cmdobjs[i]._used == true)
        {
            for (int j = 0 ; j < MAX_CODE_NUM; ++ j)
            {
                if (newPool._cmdobjs[i]._codeobjs[j]._used == true)
                {
                    data.code = j; 
                    const CodeObj& oldCode = oldPool._cmdobjs[i]._codeobjs[j];
                    const CodeObj& newCode = newPool._cmdobjs[i]._codeobjs[j];
                    data.count = STATMGR_VAL_READ(newCode._count) - 
                                 STATMGR_VAL_READ(oldCode._count);
                    if (data.count == 0)
                        data.timecost = 0;
                    else
                    {
                        data.timecost = STATMGR_VAL_READ(newCode._tmcostsum) -
                                        STATMGR_VAL_READ(oldCode._tmcostsum);
                        data.timecost /= data.count;
                    }

                    for (int k = 0; k < MAX_TIMEMAP_SIZE; ++ k)
                    {
                        data.timemap[k] = newCode._timecost[k] - oldCode._timecost[k];
                    }
                    if (_cbFuncProc != NULL)
                        _cbFuncProc((void*)&data);
                }
            }
        }
    }
    return 0;
}
int CStatMoniMgr::initPool(StatMgrPool* &pool, const char* filepath)
{
    if (filepath == NULL || access(filepath, R_OK))                                         
    {                                                                                       
        return -1;                                                           
    }                                                                                       
    int fd = ::open(filepath, O_RDONLY, 0666);                                              
    if (fd < 0)                                                                   
    {                                                                                       
        return -1;                                                           
    }                                                                                       
    pool = (StatMgrPool*) ::mmap(NULL, GET_MGR_POOL_SIZE, PROT_READ , MAP_SHARED, fd, 0);  
    if ((MAP_FAILED == pool))                                                      
    {                                                                                       
        ::close (fd);                                                                       
        pool = NULL;                                                                       
        return -1;                                                            
    }                                                                                       
    if (pool->_magic != STAT_MGR_MAGICNUM)         
    {                                                                                       
        ::close(fd);                                                                        
        ::munmap(pool, GET_MGR_POOL_SIZE);                                                 
        pool = NULL;                                                                       
        return -1;                                                                         
    }                                                                                       
    ::close(fd);                                                                            

    return 0;                                                                               
}
