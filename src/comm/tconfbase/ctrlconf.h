#ifndef _SPP_CTRLCONF_H__
#define _SPP_CTRLCONF_H__
#include "moduleconfbase.h"
#include "loadxml.h"
namespace spp
{
namespace tconfbase
{
class CCtrlConf : public CModuleConfBase
{
    public:
        CCtrlConf(CLoadConfBase* pConf);
        virtual ~CCtrlConf();

        int init();
        Report getReport(){return _report;}
        Procmon getProcmon(){return _procmon;}
        Ctrl getCtrl(){return _ctrl;}

        int getAndCheckFlog(Log& flog);
        int getAndCheckFStat(Stat& fstat);
        int getAndCheckMoni(Moni& moni);
        int getAndCheckReport(Report& report);
        int getAndCheckProcmon(Procmon& procmon);
        int getAndCheckCtrl(Ctrl& ctrl);
        int getAndCheckException(Exception &exception);

    protected:
        int loadFlog();
        int loadFStat();
        int loadMoni();
        int loadReport();
        int loadProcmon();
        int loadCtrl();
	int loadException();

        int checkReport();
        int checkProcmon();
        int checkFlog();
        int checkCtrl();


        Ctrl _ctrl;
        Report _report;
        Procmon _procmon;
		Exception _exception;
    private:

};
}
}
#endif
