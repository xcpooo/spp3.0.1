/*
 * =====================================================================================
 *
 *       Filename:  MsgBase_unittest.cpp
 *
 *    Description:  CMsgBase类单元测试
 *
 *        Version:  1.0
 *        Created:  11/24/2010 02:44:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */


#include <gtest/gtest.h>
#include <async_frame/MsgBase.h>

USING_ASYNCFRAME_NS;

/**
 * CMsgBaseTest
 */
class CMsgBaseTest
: public testing::Test
{
    public:
        CMsgBaseTest()
        {
        }
        virtual ~CMsgBaseTest()
        {
        }

    protected:
        CMsgBase _msg;
};

/**
 * 测试SetFlow/GetFlow
 */
TEST_F(CMsgBaseTest, SetAndGetFlow)
{
    int flow = -1;
    flow = _msg.GetFlow();
    EXPECT_EQ( -1, flow );

    _msg.SetFlow( 1000 );

    flow = _msg.GetFlow();

    EXPECT_EQ( 1000, flow );
}


/**
 * 测试SetInfoFlag/GetInfoFlag
 */
TEST_F(CMsgBaseTest, SetAndGetInfoFlag)
{
    bool flag = false;
    flag = _msg.GetInfoFlag();
    EXPECT_FALSE( flag );

    _msg.SetInfoFlag( true );

    flag = _msg.GetInfoFlag();
    EXPECT_TRUE( flag );

    _msg.SetInfoFlag( false );

    flag = _msg.GetInfoFlag();
    EXPECT_FALSE( flag );
}

/**
 * SetInfoFlag(false)情况下，测试AppendInfo/GetDetailInfo
 */
TEST_F(CMsgBaseTest, TestInfoIfFlagIsFalse)
{
    _msg.SetInfoFlag( false );

    _msg.AppendInfo( "xxxxx" );

    std::string info;
    _msg.GetDetailInfo( info );

    EXPECT_TRUE( info.empty() );

}


/**
 * SetInfoFlag(true)情况下，测试AppendInfo/GetDetailInfo
 */
TEST_F(CMsgBaseTest, TestInfoIfFlagIsTrue)
{
    _msg.SetInfoFlag( true );

    _msg.AppendInfo( "xxxxx" );

    std::string info;
    _msg.GetDetailInfo( info );

    EXPECT_FALSE( info.empty() );
    EXPECT_STREQ( "xxxxx", info.c_str() );

    _msg.AppendInfo( "yyyyy" );
    _msg.GetDetailInfo( info );

    EXPECT_FALSE( info.empty() );
    EXPECT_STREQ( "xxxxxyyyyy", info.c_str() );

}


/**
 * 测试SetMsgTimeout/GetMsgTimeout
 */
TEST_F(CMsgBaseTest, SetAndGetMsgTimeout)
{
    int timeout = _msg.GetMsgTimeout();
    EXPECT_EQ( 0, timeout );

    _msg.SetMsgTimeout( 63 );
    timeout = _msg.GetMsgTimeout();
    EXPECT_EQ( 63, timeout );

}
