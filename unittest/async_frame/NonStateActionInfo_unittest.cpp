/*
 * =====================================================================================
 *
 *       Filename:  NonStateActionInfo_unittest.cpp
 *
 *    Description:  CNonStateActionInfo类单元测试
 *
 *        Version:  1.0
 *        Created:  11/24/2010 11:26:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */


#include <gtest/gtest.h>
#include <async_frame/NonStateActionInfo.h>

USING_ASYNCFRAME_NS;

/**
 * CNonStateActionInfo
 */
class CNonStateActionInfoTest 
    : public testing::Test
{
    public:
        CNonStateActionInfoTest()
            : _ai(512)
        {
        }
        virtual ~CNonStateActionInfoTest()
        {
        }

    protected:
        CNonStateActionInfo _ai;
};


/**
 * 测试CNonStateActionInfo::SetRouteID和CNonStateActionInfo::GetRouteID
 *
 * 设置的modid,cmdid和获取到的modid,cmdid应该是一样的
 */
TEST_F(CNonStateActionInfoTest, SetAndGetRouteID)
{
    int modid, cmdid;

    modid = 100;
    cmdid = 999;
    _ai.SetRouteID(modid, cmdid);

    int tmp_modid, tmp_cmdid;
    tmp_modid = tmp_cmdid = -1;

    _ai.GetRouteID( tmp_modid, tmp_cmdid);

    EXPECT_EQ( modid, tmp_modid );
    EXPECT_EQ( cmdid, tmp_cmdid );
    
}

/**
 * 测试CNonStateActionInfo::GetRouteType
 *
 * route type应该为L5NonStateRoute
 */
TEST_F(CNonStateActionInfoTest, GetRouteType)
{
    RouteType type = _ai.GetRouteType();
    EXPECT_EQ( L5NonStateRoute, type);

}
