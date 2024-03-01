#include "gtest/gtest.h"
#include "unittesteq.h"
#include "tconfbase/sessionmgrconf.h"
#include "tlog.h"

using namespace spp::tconfbase;
class CMgrConf
    : public testing::Test
{
    public:
        CMgrConf()
        {
            SingleTon<CTLog, CreateByProto>::SetProto(&flog);
            return;
        }
        virtual ~CMgrConf()
        {
            return;
        }
        

    protected:
        CTLog flog;
        CSessionMgrConf* mgr;
};
TEST_F(CMgrConf, CMgrConfTest)
{
    mgr = new CSessionMgrConf(new CLoadXML("./tconfbase/session.xml"));
    ASSERT_EQ (0, mgr->init());
    AsyncSession session;
    AsyncSessionEntry entry;
    entry.id = 0;
    entry.maintype = "main";
    entry.subtype = "sub";
    entry.ip = "1.0.0.1";
    entry.port = 10000;
    entry.recv_count = 5;
    entry.timeout = 3;
    entry.multi_con_inf = 10;
    entry.multi_con_sup = 50;
    session.entry.push_back(entry);
    entry.id = 1;
    entry.multi_con_inf = 1;
    entry.multi_con_sup = 1;
    session.entry.push_back(entry);

    AsyncSession _session;
    ASSERT_EQ(0, mgr->getAndCheckAsyncSession(_session));

    ASSERT_EQ(1, equalAsyncSession(session, _session));
    delete mgr;

}

