#ifndef _SPP_MODULECONFBASE_H__
#define _SPP_MODULECONFBASE_H__

#include "loadconfbase.h"
#include "singleton.h"

namespace spp
{
namespace tconfbase
{
enum ERR_MODULE_CONF_BASE
{
    ERR_MODULE_CONF_SUCCESS = 0,
    ERR_CONF_CHECK_UNPASS = ERR_END + 1,
    ERR_LOAD_VALUE,
};
class CModuleConfBase
{
    public:
        CModuleConfBase(CLoadConfBase* pConf);
        virtual ~CModuleConfBase(); 
        int init(); 
        
        virtual int loadFlog()=0;
        virtual int loadFStat()=0;
        virtual int loadMoni()=0;

        virtual int checkFStat();
        virtual int checkFlog();
        virtual int checkMoni();

        Stat getFStat() {return _fstat;};
        Log getFlog() {return _flog;};
        Moni getMoni() {return _moni;};
 
    protected:
        
        int checklog(const std::string& key,const Log& log);
        std::vector<std::string> getArgvs(int count, ...);

        Stat _fstat;
        Log _flog;
        Moni _moni;

       CLoadConfBase* _pLoadConf;
    private:
};
}
}
#endif
