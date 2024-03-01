#include "gtest/gtest.h"
#include "unittesteq.h"
#include "tconfbase/proxyconf.h"
#include "tlog.h"

using namespace spp::tconfbase;
class CProxyConfInitTest
    : public testing::Test
{
    public:
        CProxyConfInitTest()
        {
            SingleTon<CTLog, CreateByProto>::SetProto(&flog);
            return;
        }
        virtual ~CProxyConfInitTest()
        {
            return;

        }

    protected:
        CTLog flog;
        CProxyConf* proxy;
};
/**
 * 测试xml文件的初始化 
 *
 * 当文件不存在应返回错误
 */
TEST_F(CProxyConfInitTest, CProxyConfInit) 
{
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));
    EXPECT_EQ (0, proxy->init());
    delete proxy;    
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxyxml"));
    EXPECT_EQ (ERR_LOAD_CONF_FILE, proxy->init());
    delete proxy;
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy1.xml"));
    EXPECT_EQ (ERR_NO_SUCH_ELEM, proxy->init());
    delete proxy;
}
/**
 * 测试读取FLOG的配置
 *
 */
TEST_F(CProxyConfInitTest, CProxyConfFlog)
{
    Log log;
    log.clear("spp_frame_proxy");
    log.maxfilesize = 10240000;
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));
    proxy->init();

    EXPECT_EQ(0, proxy->loadFlog());
    EXPECT_EQ(0, proxy->checkFlog());
    EXPECT_EQ(1, equalLog(proxy->getFlog(), log));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy3.xml"));
    proxy->init();
    log.level = 4;
    log.prefix = "spp_frame_proxy";
    log.maxfilesize = 10485760;
    log.path = "log";
    EXPECT_EQ(0, proxy->loadFlog());
    EXPECT_EQ(0, proxy->checkFlog());
    EXPECT_EQ(1, equalLog(proxy->getFlog(), log));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy1.xml"));
    proxy->init();
    EXPECT_EQ(ERR_NO_SUCH_ELEM, proxy->loadFlog()); 
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy2.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadFlog()); 
    EXPECT_EQ(ERR_CONF_CHECK_UNPASS, proxy->checkFlog());
    delete proxy;
}

/*
 * 测试读取FSTAT配置
 */
TEST_F(CProxyConfInitTest, CProxyConfFStat)
{
    Stat stat;
    stat.clear("../stat/stat_spp_proxy.dat");
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadFStat());
    EXPECT_EQ(0, proxy->checkFStat());
    EXPECT_EQ(1, equalStat(proxy->getFStat(), stat));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy3.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadFStat());    
    EXPECT_EQ(0, proxy->checkFStat());   
    EXPECT_EQ(1, equalStat(proxy->getFStat(), stat));
    delete proxy;
}

/*
 * 测试读取MONI配置
 */ 

TEST_F(CProxyConfInitTest, CProxyConfMoni)
{
    Moni moni;
    moni.clear("moni_spp_proxy");
    moni.log.path = "../moni";
    moni.log.maxfilesize = 10240000;
    moni.log.level = LOG_NORMAL;
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadMoni());
    EXPECT_EQ(0, proxy->checkMoni());
    EXPECT_EQ(1, equalMoni(moni, proxy->getMoni()));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy3.xml"));
    moni.log.prefix = "slow fuck";
    proxy->init();
    EXPECT_EQ(0, proxy->loadMoni());
    EXPECT_EQ(0, proxy->checkMoni());
    EXPECT_EQ(1, equalMoni(moni, proxy->getMoni()));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy1.xml"));
    proxy->init();
    EXPECT_EQ(ERR_NO_SUCH_ELEM, proxy->loadMoni());

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy2.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadMoni());
    EXPECT_EQ(ERR_CONF_CHECK_UNPASS, proxy->checkMoni());


}
/*
 * 测试读取acceptor配置项
 */ 
TEST_F(CProxyConfInitTest, CProxyConfAcceptor)
{
    AcceptorSock acc;
    acc.udpclose = 1;

    AcceptorEntry entry;
    entry.port = 10085;
    entry.TOS = 123;
    entry.type = "tcp";
    acc.entry.push_back(entry);
    entry.TOS = -1;
    entry.type = "udp";
    acc.entry.push_back(entry);
    entry.type = "unix";
    entry.path = "/tmp/spp_proxy_unix";
    entry.port = 0;
    acc.entry.push_back(entry);
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));

    proxy->init();
    EXPECT_EQ(0, proxy->loadAcceptor());
    EXPECT_EQ(0, proxy->checkAcceptor());
    EXPECT_EQ(1, equalAcceptorSock(acc, proxy->getAcceptor()));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy2.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadAcceptor());
    EXPECT_EQ(ERR_CONF_CHECK_UNPASS, proxy->checkAcceptor());
    delete proxy;

}
TEST_F(CProxyConfInitTest, CProxyConfConnector)
{
    ConnectShm conn;
    ConnectEntry entry;
    entry.id = 1;
    conn.entry.push_back(entry);
    entry.id = 2;
    conn.entry.push_back(entry);
    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy.xml"));

    proxy->init();
    EXPECT_EQ(0, proxy->loadConnector());
    EXPECT_EQ(0, proxy->checkConnector());
    EXPECT_EQ(1,equalConnectShm(conn, proxy->getConnector()));
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy2.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadConnector());
    EXPECT_EQ(ERR_CONF_CHECK_UNPASS, proxy->checkConnector());
    delete proxy;

    proxy = new CProxyConf(new CLoadXML("./tconfbase/proxy3.xml"));
    proxy->init();
    EXPECT_EQ(0, proxy->loadConnector());
    EXPECT_EQ(0, proxy->checkConnector());
    EXPECT_EQ(1,equalConnectShm(conn, proxy->getConnector()));
    delete proxy;
}


