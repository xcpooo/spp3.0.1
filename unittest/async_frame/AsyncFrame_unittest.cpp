/*
 * =====================================================================================
 *
 *       Filename:  AsyncFrame_unittest.cpp
 *
 *    Description:  CAsyncFrame单元测试
 *
 *        Version:  1.0
 *        Created:  11/25/2010 05:19:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/AsyncFrame.h>
#include <async_frame/IState.h>

USING_ASYNCFRAME_NS;

/**
 * 测试CAsyncFrame时使用的IState实现
 */
class CStateForTestAsyncFrame
    : public IState
{
    public:
        virtual ~CStateForTestAsyncFrame()
        {
        }

        virtual int HandleEncode(CAsyncFrame *pFrame,
                CActionSet *pActionSet,
                CMsgBase *pMsgBase)
        {
            return 0;
        }

        virtual int HandleProcess(CAsyncFrame *pFrame,
                CActionSet *pActionSet,
                CMsgBase *pMsgBase)
        {
            return 0;
        }
};

/**
 * CAsyncFrameTest
 */
class CAsyncFrameTest
    : public testing::Test
{
    public:
        CAsyncFrameTest()
        {
        }

        virtual ~CAsyncFrameTest()
        {
        }

    protected:
        CAsyncFrame _asyncFrame;

};

/**
 * 测试CAsyncFrame::RegCallback需要
 */
int CBFunc(CAsyncFrame*, CMsgBase*)
{
    return 0;
}

/**
 * 测试CAsyncFrame::InitFrame
 */
TEST_F( CAsyncFrameTest, InitFrame )
{
    int ret = _asyncFrame.InitFrame(NULL, NULL, NULL, -1 );
    EXPECT_NE( 0, ret ); 
}

/**
 * 测试CAsyncFrame::InitFrame2
 */
TEST_F( CAsyncFrameTest, InitFrame2 )
{
    int ret = _asyncFrame.InitFrame2(NULL);
    EXPECT_NE( 0, ret ); 
}

/**
 * 测试CAsyncFrame::RegCallback
 */
TEST_F( CAsyncFrameTest, RegCallback )
{
    int ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Init, CBFunc );
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Fini, CBFunc );
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Overload, CBFunc );
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Init, NULL);
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Fini, NULL);
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( CAsyncFrame::CBType_Overload, NULL);
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.RegCallback( (CAsyncFrame::CBType)-1, CBFunc);
    EXPECT_NE( 0, ret );

    ret = _asyncFrame.RegCallback( (CAsyncFrame::CBType)3, CBFunc);
    EXPECT_NE( 0, ret );

}

/**
 * 测试CAsyncFrame::AddState
 */
TEST_F( CAsyncFrameTest, AddState )
{
    CStateForTestAsyncFrame *pState = new CStateForTestAsyncFrame;
    int ret = _asyncFrame.AddState(0,  pState);
    EXPECT_NE( 0, ret );

    ret = _asyncFrame.AddState(-1,  pState);
    EXPECT_NE( 0, ret );

    ret = _asyncFrame.AddState(1,  NULL);
    EXPECT_NE( 0, ret );

    ret = _asyncFrame.AddState(1,  pState);
    EXPECT_EQ( 0, ret );

    ret = _asyncFrame.AddState(1,  pState);
    EXPECT_NE( 0, ret );
}


