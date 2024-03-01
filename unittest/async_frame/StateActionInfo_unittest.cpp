/*
 * =====================================================================================
 *
 *       Filename:  StateActionInfo_unittest.cpp
 *
 *    Description:  CStateActionInfo类单元测试
 *
 *        Version:  1.0
 *        Created:  11/24/2010 11:39:46 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/StateActionInfo.h>

USING_ASYNCFRAME_NS;

/**
 * CStateActionInfoTest
 */
class CStateActionInfoTest
: public testing::Test
{
    public:
        CStateActionInfoTest()
            : _ai(512)
        {
        }
        virtual ~CStateActionInfoTest()
        {
        }

    protected:
        CStateActionInfo _ai;
};

/**
 * 测试GetRouteType
 *
 * route type应该为L5StateRoute
 *
 */
TEST_F(CStateActionInfoTest, GetRouteType)
{
    RouteType type = _ai.GetRouteType();
    EXPECT_EQ( L5StateRoute, type);
}

/**
 * 测试SetRouteKey/GetRouteKey
 */
TEST_F(CStateActionInfoTest, SetAndGetRouteKey)
{
    int modid = 1000;
    int64_t key = 98765; 
    int32_t funid = 0;

    _ai.SetRouteKey( modid, key, funid );

    int tmp_modid = -1;
    int64_t tmp_key = -1;
    int32_t tmp_funid = -1;

    _ai.GetRouteKey( tmp_modid, tmp_key, tmp_funid);

    EXPECT_EQ( modid, tmp_modid );
    EXPECT_EQ( key, tmp_key );
    EXPECT_EQ( funid, tmp_funid );
}

/**
 * 测试GetRouteID
 */
TEST_F(CStateActionInfoTest, GetRouteID)
{
    int modid = 1000;
    int64_t key = 98765; 
    int32_t funid = 0;
    int cmdid = 999;

    _ai.SetRouteKey( modid, key, funid );
    _ai.SetCmdID( cmdid );


    int tmp_modid = -1, tmp_cmdid = -1;
    _ai.GetRouteID( tmp_modid, tmp_cmdid );

    EXPECT_EQ( modid, tmp_modid );
    EXPECT_EQ( cmdid, tmp_cmdid );

}
