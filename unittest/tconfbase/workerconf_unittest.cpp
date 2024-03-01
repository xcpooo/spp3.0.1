#include "gtest/gtest.h"
#include "unittesteq.h"
#include "tconfbase/workerconf.h"
#include "tlog.h"

using namespace spp::tconfbase;
class CWorkerConfTest
    : public testing::Test
{
    public:
        CWorkerConfTest()
        {
            SingleTon<CTLog, CreateByProto>::SetProto(&flog);
            return;
        }
        virtual ~CWorkerConfTest()
        {
            return;
        }
        

    protected:
        CTLog flog;
        CWorkerConf* worker;
        CWorkerConf* worker1;
        CWorkerConf* worker2;
        CWorkerConf* worker3;
};
TEST_F(CWorkerConfTest, CWorkerConfInit)
{
    worker = new CWorkerConf(new CLoadXML("./tconfbase/worker.xml"));
    ASSERT_EQ (0, worker->init());

    worker1 = new CWorkerConf(new CLoadXML("./tconfbase/worker1.xml"));
    ASSERT_EQ (ERR_NO_SUCH_ELEM, worker1->init());

    worker2 = new CWorkerConf(new CLoadXML("./tconfbase/worker2.xml"));
    ASSERT_EQ (0, worker2->init());

    worker3 = new CWorkerConf(new CLoadXML("./tconfbase/worker3.xml"));
    ASSERT_EQ (0, worker3->init());
}

TEST_F(CWorkerConfTest, CWorkerConfFlog)
{
    Log flog;
    flog.clear("spp_frame_worker1");
    flog.maxfilesize =10240000;
    ASSERT_EQ(0, worker->loadFlog());
    ASSERT_EQ(0, worker->checkFlog());
    ASSERT_EQ(1, equalLog(flog, worker->getFlog()));

    ASSERT_EQ(0, worker2->loadFlog());
    ASSERT_EQ(0, worker2->checkFlog());
    ASSERT_EQ(1, equalLog(flog, worker2->getFlog()));
}
TEST_F(CWorkerConfTest, CWorkerConfFStat)
{
    Stat stat;
    stat.clear("../stat/stat_spp_worker1.dat");

    ASSERT_EQ(0, worker->loadFStat());
    ASSERT_EQ(0, worker->checkFStat());
    ASSERT_EQ(1, equalStat(stat, worker->getFStat()));
                  
    ASSERT_EQ(0, worker2->loadFStat());
    ASSERT_EQ(0, worker2->checkFStat());
    ASSERT_EQ(1, equalStat(stat, worker2->getFStat()));

    ASSERT_EQ(0, worker3->loadFStat());
    ASSERT_EQ(0, worker3->checkFStat());
 
}
TEST_F(CWorkerConfTest, CWorkerConfMoni)
{
    Moni moni;
    ASSERT_EQ(0, worker->loadMoni());
    ASSERT_EQ(0, worker->checkMoni());
    ASSERT_EQ(1, equalMoni(moni, worker->getMoni()));

    ASSERT_EQ(0, worker2->loadMoni());
    ASSERT_EQ(0, worker2->checkMoni());
    ASSERT_EQ(1, equalMoni(moni, worker2->getMoni()));

    ASSERT_EQ(0, worker3->loadMoni());
    ASSERT_EQ(ERR_CONF_CHECK_UNPASS, worker3->checkMoni());

}
TEST_F(CWorkerConfTest, CWorkerConfStat)
{
    Stat stat;
    stat.file = "../stat/module_stat_spp_worker1.dat";
    ASSERT_EQ(0, worker->loadStat());
    ASSERT_EQ(0, worker->checkStat());
    ASSERT_EQ(1, equalStat(stat, worker->getStat()));

    ASSERT_EQ(0, worker2->loadStat());
    ASSERT_EQ(0, worker2->checkStat());
    ASSERT_EQ(1, equalStat(stat, worker2->getStat()));

    ASSERT_EQ(0, worker3->loadStat());
    ASSERT_EQ(0, worker3->checkStat());

}
TEST_F(CWorkerConfTest, CWorkerConfAcceptor)
{
    ConnectShm conn;
    conn.entry.push_back(ConnectEntry());

    ASSERT_EQ(0, worker->loadAcceptor());
    ASSERT_EQ(0, worker->checkAcceptor());
    ASSERT_EQ(1, equalConnectShm(conn, worker->getAcceptor()));

    ASSERT_EQ(0, worker2->loadAcceptor());
    ASSERT_EQ(0, worker2->checkAcceptor());
    ASSERT_EQ(1,equalConnectShm(conn, worker2->getAcceptor()));

    ASSERT_EQ(0, worker3->loadAcceptor());
    ASSERT_EQ(0, worker3->checkAcceptor());

}
TEST_F(CWorkerConfTest, CWorkerConfModule)
{

    Module module;
    module.bin = "./spp_module_test.so";
    module.etc = "../etc/module.conf";

    ASSERT_EQ(0, worker->loadModule());
    ASSERT_EQ(0, worker->checkModule());
    ASSERT_EQ(1, equalModule(module, worker->getModule()));

    ASSERT_EQ(0, worker2->loadModule());
    ASSERT_EQ(0, worker2->checkModule());
    ASSERT_EQ(1,equalModule(module, worker2->getModule()));


}

TEST_F(CWorkerConfTest, JUSTDELETE)
{
    delete worker1;
    delete worker;
    delete worker2;
    delete worker3;

}
