/*
 * =====================================================================================
 *
 *       Filename:  ActionInfo_unittest.cpp
 *
 *    Description:  CActionInfo单元测试
 *
 *        Version:  1.0
 *        Created:  11/23/2010 10:16:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shak_aibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/ActionInfo.h>

USING_ASYNCFRAME_NS;

class CActionInfoTest 
    : public testing::Test
{
    public:
        CActionInfoTest()
            : _ai(512)
        {
        }
        virtual ~CActionInfoTest()
        {
        }

    protected:
        CActionInfo _ai;
};

/**
 * 测试CActionInfo::SetDestIp
 * 
 */
TEST_F(CActionInfoTest, SetDestIp)
{
    char szIP[20] = "10.6.207.128";

    int ret = _ai.SetDestIp( szIP );
    EXPECT_EQ( 0, ret );

    ret = _ai.SetDestIp( NULL );
    EXPECT_NE( 0, ret );
    
}

/**
 * 测试CActionInfo::GetDestIp
 *
 * 设置的IP和获取到的IP应该是一样的
 */
TEST_F(CActionInfoTest, GetDestIp)
{
    char szIP[20] = "10.6.207.128";

    int ret = _ai.SetDestIp( szIP );
    ASSERT_EQ( 0, ret );

    std::string ip;
    _ai.GetDestIp( ip );

    EXPECT_STREQ( szIP, ip.c_str() );
    
}

/**
 * 测试CActionInfo::SetDestPort和CActionInfo::GetDestPort
 *
 * 设置的Port和获取到的Port应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetDestPort)
{
    unsigned short port = 8888;
    _ai.SetDestPort( port );

    unsigned short port2 = 0;
    _ai.GetDestPort( port2 );
    EXPECT_EQ( port, port2 );
    
}

/**
 * 测试CActionInfo::SetProto和CActionInfo::GetProto
 *
 * 设置的proto和获取到的proto应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetProto)
{
    ProtoType type = ProtoType_TCP;
    _ai.SetProto( type );

    ProtoType type2 = ProtoType_Invalid;
    _ai.GetProto( type2);
    EXPECT_EQ( type, type2);
    
}

/**
 * 测试CActionInfo::SetActionType和CActionInfo::GetActionType
 *
 * 设置的type和获取到的type应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetActionType)
{
    ActionType type = ActionType_SendRecv;
    _ai.SetActionType( type );

    ActionType type2 = ActionType_Invalid;
    _ai.GetActionType( type2);
    EXPECT_EQ( type, type2);
    
}

/**
 * 测试CActionInfo::SetID和CActionInfo::GetID
 *
 * 设置的id和获取到的id应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetID)
{
    int id = 1;
    _ai.SetID( id );

    int id2 = 0;
    _ai.GetID( id2 );
    EXPECT_EQ( id, id2 );
    
}


/**
 * 测试CActionInfo::SetTimeout和CActionInfo::GetTimeout
 *
 * 设置的timeout和获取到的timeout应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetTimeout)
{
    int timeout = 10;
    _ai.SetTimeout( timeout );

    int timeout2 = 0;
    _ai.GetTimeout( timeout2 );
    EXPECT_EQ( timeout, timeout2 );
    
}

/**
 * 测试CActionInfo::SetErrno和CActionInfo::GetErrno
 *
 * 设置的errno和获取到的errno应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetErrno)
{
    int err = -10;
    _ai.SetErrno( err );

    int err2 = 0;
    _ai.GetErrno( err2 );
    EXPECT_EQ( err, err2 );
    
}

/**
 * 测试CActionInfo::SetTimeCost和CActionInfo::GetTimeCost
 *
 * 设置的timecost和获取到的timecost应该是一样的
 */
TEST_F(CActionInfoTest, SetAndGetTimeCost)
{
    int cost = 99;
    _ai.SetTimeCost( cost );

    int cost2 = 0;
    _ai.GetTimeCost( cost2 );
    EXPECT_EQ( cost, cost2 );
    
}

/**
 * 测试CActionInfo::GetBuffer
 *
 */
TEST_F(CActionInfoTest, GetBuffer)
{
    char *buf = NULL;
    int size = -1;

    _ai.GetBuffer( &buf, size);

    EXPECT_NE( 0, (long long)buf );
    EXPECT_EQ( 0, size);
    
}

/**
 * 测试CActionInfo::GetRouteType
 *
 */
TEST_F(CActionInfoTest, GetRouteType)
{
    RouteType type = _ai.GetRouteType();
    EXPECT_EQ( NoGetRoute, type);
    
}
