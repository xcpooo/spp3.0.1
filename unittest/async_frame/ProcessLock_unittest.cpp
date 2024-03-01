/*
 * =====================================================================================
 *
 *       Filename:  ProcessLock_unittest.cpp
 *
 *    Description:  CProcessLock单元测试
 *
 *        Version:  1.0
 *        Created:  11/23/2010 04:00:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <gtest/gtest.h>
#include <async_frame/ProcessLock.h>
#include <file_utility.h>

using namespace PROCESS_LOCK;

////////////////////////////////////////////////////////////
/**
 * mapfile为NULL的CProcessLock测试
 *
 */
class CProcessLockTestWithMapfileIsNULL
    : public testing::Test
{
    public:
        CProcessLockTestWithMapfileIsNULL()
            : _mapfile(NULL)
        {
        }
        virtual ~CProcessLockTestWithMapfileIsNULL()
        {
        }

    protected:
        virtual void SetUp()
        {
        }
        virtual void TearDown()
        {
        }

        char *_mapfile;
};

/**
 * mapfile为NULL时，Init/Lock/UnLock方法执行失败
 */
TEST_F(CProcessLockTestWithMapfileIsNULL, FailIfMapfileIsNULL) 
{
    CProcessLock lock;

    int ret = lock.Init(_mapfile);
    ASSERT_NE(0, ret );

    ret = lock.Lock();
    ASSERT_NE(0, ret );

    ret = lock.UnLock();
    ASSERT_NE(0, ret );

}
////////////////////////////////////////////////////////////
/**
 * mapfile非NULL,并且存在情况下的CProcessLock测试
 *
 */
class CProcessLockTestWithMapfileExist
    : public testing::Test
{
    public:
        CProcessLockTestWithMapfileExist()
            : _mapfile(NULL)
        {
            _mapfile = new char[1024];
            strncpy( _mapfile, "/tmp/process_lock_mapfile.lock", 1023 );
        }
        virtual ~CProcessLockTestWithMapfileExist()
        {
            delete [] _mapfile;
        }

    protected:
        virtual void SetUp()
        {
            CFileUtility::TouchFile( _mapfile );
        }
        virtual void TearDown()
        {
            CFileUtility::RmFile( _mapfile );
        }

        char *_mapfile;
};

/**
 * mapfile非NULL,并且存在情况下，Init/Lock/UnLock方法执行成功
 */
TEST_F(CProcessLockTestWithMapfileExist, SuccessIfMapfileExist) 
{
    bool exists = CFileUtility::IsExist( _mapfile );
    ASSERT_EQ(true, exists);

    CProcessLock lock;

    int ret = lock.Init(_mapfile);
    ASSERT_EQ(0, ret );

    ret = lock.Lock();
    ASSERT_EQ(0, ret );

    ret = lock.UnLock();
    ASSERT_EQ(0, ret );

}

////////////////////////////////////////////////////////////
/**
 * mapfile非NULL,并且不存在情况下的CProcessLock测试
 *
 */
class CProcessLockTestWithMapfileNotExist
    : public testing::Test
{
    public:
        CProcessLockTestWithMapfileNotExist()
            : _mapfile(NULL)
        {
            _mapfile = new char[1024];
            strncpy( _mapfile, "/tmp/process_lock_mapfile.lock", 1023 );
        }
        virtual ~CProcessLockTestWithMapfileNotExist()
        {
            delete [] _mapfile;
        }

    protected:
        virtual void SetUp()
        {
            CFileUtility::RmFile( _mapfile );
        }
        virtual void TearDown()
        {
            CFileUtility::RmFile( _mapfile );
        }

        char *_mapfile;
};

/**
 * mapfile非NULL,并且不存在情况下，Init/Lock/UnLock方法执行成功
 */
TEST_F(CProcessLockTestWithMapfileNotExist, SuccessIfMapfileNotExist) 
{
    bool exists = CFileUtility::IsExist( _mapfile );
    ASSERT_EQ(false, exists);

    CProcessLock lock;

    int ret = lock.Init(_mapfile);
    ASSERT_EQ(0, ret );

    ret = lock.Lock();
    ASSERT_EQ(0, ret );

    ret = lock.UnLock();
    ASSERT_EQ(0, ret );

}

////////////////////////////////////////////////////////////

