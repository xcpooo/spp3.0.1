#include "gtest/gtest.h"
#include "unittesteq.h"
#include "tconfbase/ctrlconf.h"
#include "tlog.h"

using namespace spp::tconfbase;
class CCtrlConfTest
    : public testing::Test
{
    public:
        CCtrlConfTest()
        {
            SingleTon<CTLog, CreateByProto>::SetProto(&flog);
            return;
        }
        virtual ~CCtrlConfTest()
        {
            return;
        }
        

    protected:
        CTLog flog;
        CCtrlConf* ctrl;
        CCtrlConf* ctrl1;
        CCtrlConf* ctrl2;
        CCtrlConf* ctrl3;
};
TEST_F(CCtrlConfTest, CCtrlConfInit)
{
    ctrl = new CCtrlConf(new CLoadXML("./tconfbase/ctrl.xml"));
    ASSERT_EQ (0, ctrl->init());

    ctrl1 = new CCtrlConf(new CLoadXML("./tconfbase/ctrl1xml"));
    ASSERT_EQ (ERR_LOAD_CONF_FILE, ctrl1->init());

    ctrl2 = new CCtrlConf(new CLoadXML("./tconfbase/ctrl2.xml"));
    ASSERT_EQ (0, ctrl2->init());

    ctrl3 = new CCtrlConf(new CLoadXML("./tconfbase/ctrl3.xml"));
    ASSERT_EQ (0, ctrl3->init());
}

TEST_F(CCtrlConfTest, CCtrlConfPromon)
{
    Procmon procmon;
    ProcmonEntry entry;
    entry.id = 0;
    entry.exe = "spp_proxy";
    entry.etc = "../etc/spp_proxy.xml";
    procmon.entry.push_back(entry);
    entry.id = 1;
    entry.exe = "spp_worker";
    entry.etc = "../etc/spp_worker1.xml";
    procmon.entry.push_back(entry);
    entry.id = 2;
    entry.exe = "spp_worker";
    entry.etc = "../etc/spp_worker2.xml";
    procmon.entry.push_back(entry);

    ASSERT_EQ(0, ctrl->loadProcmon());
    ASSERT_EQ(0, ctrl->checkProcmon());
    ASSERT_EQ(1, equalProcmon(procmon, ctrl->getProcmon()));

    ASSERT_EQ(0, ctrl2->loadProcmon());
    ASSERT_EQ(ERR_CONF_CHECK_UNPASS, ctrl2->checkProcmon());

    ASSERT_EQ(0, ctrl3->loadProcmon());
    ASSERT_EQ(ERR_CONF_CHECK_UNPASS, ctrl3->checkProcmon());
 

}
TEST_F(CCtrlConfTest, JUSTDELETE)
{
    delete ctrl;
    delete ctrl1;
    delete ctrl2;
    delete ctrl3;
}
