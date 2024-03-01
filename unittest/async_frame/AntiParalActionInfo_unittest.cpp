/*
 * =====================================================================================
 *
 *       Filename:  AntiParalAction_unittest.cpp
 *
 *    Description:  CAntiParalActionInfo类单元测试
 *
 *        Version:  1.0
 *        Created:  11/24/2010 11:51:54 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/AntiParalActionInfo.h>

USING_ASYNCFRAME_NS;

/**
 * CAntiParalActionInfoTest
 */
class CAntiParalActionInfoTest
: public testing::Test
{
    public:
        CAntiParalActionInfoTest()
            : _ai(512)
        {
        }
        virtual ~CAntiParalActionInfoTest()
        {
        }

    protected:
        CAntiParalActionInfo _ai;
};

/**
 * 测试GetRouteType
 *
 */
TEST_F(CAntiParalActionInfoTest, GetRouteType)
{
    RouteType type = _ai.GetRouteType();
    EXPECT_EQ( L5AntiParalRoute, type);
}

/**
 * 测试GetRouteKey
 *
 */
TEST_F(CAntiParalActionInfoTest, GetRouteKey)
{
    int modid, cmdid;
    int64_t key;
    modid = 1111;
    cmdid = 2222;
    key = 123456789;

    _ai.SetRouteKey( modid, cmdid, key );

    int tmp_modid = -1, tmp_cmdid = -1;
    int64_t tmp_key = -1;

    _ai.GetRouteKey( tmp_modid, tmp_cmdid, tmp_key );

    EXPECT_EQ( modid, tmp_modid );
    EXPECT_EQ( cmdid, tmp_cmdid );
    EXPECT_EQ( key, tmp_key );
}

/**
 * 测试GetRouteID
 *
 */
TEST_F(CAntiParalActionInfoTest, GetRouteID)
{
    int modid, cmdid;
    int64_t key;
    modid = 1234;
    cmdid = 5678;
    key = 123456789;

    _ai.SetRouteKey( modid, cmdid, key );

    int tmp_modid = -1, tmp_cmdid = -1;

    _ai.GetRouteID( tmp_modid, tmp_cmdid );

    EXPECT_EQ( modid, tmp_modid );
    EXPECT_EQ( cmdid, tmp_cmdid );
}

/**
 * 测试SetIdealHost/GetIdealHost
 *
 */
TEST_F(CAntiParalActionInfoTest, SetAndGetIdealHost)
{
    std::string ip("10.6.207.128");
    PortType port = 5574;

    _ai.SetIdealHost( ip, port );

    std::string tmp_ip;
    PortType tmp_port = 0;
    _ai.GetIdealHost( tmp_ip, tmp_port );

    EXPECT_STREQ( ip.c_str(), tmp_ip.c_str() );
    EXPECT_EQ( port, tmp_port );
}


