/*
 * =====================================================================================
 *
 *       Filename:  ActionSet_unittest.cpp
 *
 *    Description:  CActionSet单元测试
 *
 *        Version:  1.0
 *        Created:  11/24/2010 03:10:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/ActionSet.h>
#include <async_frame/IState.h>
#include <async_frame/AsyncFrame.h>
#include <async_frame/MsgBase.h>
#include <async_frame/ActionInfo.h>

USING_ASYNCFRAME_NS;

/**
 * 测试CActionSet时临时的IState实现
 */
class CStateForTestActionSet
    : public IState
{
    public:
        virtual ~CStateForTestActionSet()
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
 * CActionSetTest
 */
class CActionSetTest
    : public testing::Test
{
    public:
        CActionSetTest()
            : _async_frame(NULL), _state(NULL), _msg(NULL), _ai_set(NULL)
        {
            _async_frame = new CAsyncFrame;
            _state = (IState *)new CStateForTestActionSet;
            _msg = new CMsgBase;
            _ai_set = new CActionSet( _async_frame, _state, _msg );
        }
        virtual ~CActionSetTest()
        {
            delete _ai_set;
            delete _msg;
            delete _state;
            delete _async_frame;
        }

    protected:
        CAsyncFrame *_async_frame;
        IState * _state;
        CMsgBase *_msg;
        CActionSet *_ai_set;

};

/**
 * 测试AddAction
 */
TEST_F( CActionSetTest, AddAction )
{
    int ret = _ai_set->AddAction(NULL);
    EXPECT_NE( 0, ret );

    CActionInfo *ai = new CActionInfo(512);
    ret = _ai_set->AddAction( ai );
    EXPECT_EQ( 0, ret );
}

/**
 * 测试GetActionNum
 */
TEST_F( CActionSetTest, GetActionNum )
{
    int num = _ai_set->GetActionNum();
    EXPECT_EQ( 0, num );

    CActionInfo *ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 1, num );

    ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 2, num );


    ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 3, num );
}
/**
 * 测试ClearAction
 */
TEST_F( CActionSetTest, ClearAction )
{
    CActionInfo *ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    int num = _ai_set->GetActionNum();
    EXPECT_EQ( 1, num );

    ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 2, num );

    _ai_set->ClearAction();
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 0, num );

    ai = new CActionInfo(512);
    _ai_set->AddAction( ai );
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 1, num );

    _ai_set->ClearAction();
    num = _ai_set->GetActionNum();
    EXPECT_EQ( 0, num );

}
