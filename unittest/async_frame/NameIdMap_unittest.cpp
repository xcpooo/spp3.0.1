/*
 * =====================================================================================
 *
 *       Filename:  NameIdMap_unittest.cpp
 *
 *    Description:  CNameIdMap单元测试
 *
 *        Version:  1.0
 *        Created:  11/23/2010 09:00:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/NameIdMap.h>
#include <file_utility.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

USING_L5MODULE_NS;

////////////////////////////////////////////////////////////
/**
 * lock_file和caller_table_file为NULL的CNameIdMap测试
 *
 */
class CNameIdMapTestWithBothfilesAreNULL
    : public testing::Test
{
    public:
        CNameIdMapTestWithBothfilesAreNULL()
            : _lock_file(NULL), _caller_table_file(NULL)
        {
        }
        virtual ~CNameIdMapTestWithBothfilesAreNULL()
        {
        }

    protected:
        virtual void SetUp()
        {
        }
        virtual void TearDown()
        {
        }

        char *_lock_file;
        char *_caller_table_file;
};

/**
 * lock_file和caller_table_file为NULL，Init/GetID/GetCallerInfo方法执行失败
 */
TEST_F(CNameIdMapTestWithBothfilesAreNULL, FailIfBothfilesAreNULL) 
{
    CNameIdMap name_id_map;

    int ret = name_id_map.Init(_lock_file, _caller_table_file);
    ASSERT_NE(0, ret );

    ret = name_id_map.GetID();
    ASSERT_GT(0, ret );

    caller_name_t caller_info;
    ret = name_id_map.GetCallerInfo(1, caller_info);
    ASSERT_NE(0, ret );

}
////////////////////////////////////////////////////////////
/**
 * lock_file为NULL, caller_table_file非NULL的CNameIdMap测试
 *
 */
class CNameIdMapTestWithLockfileIsNULL
    : public testing::Test
{
    public:
        CNameIdMapTestWithLockfileIsNULL()
            : _lock_file(NULL), _caller_table_file(NULL)
        {
            _caller_table_file = new char[1024];
            strncpy( _caller_table_file, "/tmp/caller_table.bin", 1023);
        }
        virtual ~CNameIdMapTestWithLockfileIsNULL()
        {
            delete [] _caller_table_file;
        }

    protected:
        virtual void SetUp()
        {
        }
        virtual void TearDown()
        {
        }

        char *_lock_file;
        char *_caller_table_file;
};

/**
 * lock_file为NULL, caller_table_file非NULL时，Init/GetID/GetCallerInfo方法执行失败
 */
TEST_F(CNameIdMapTestWithLockfileIsNULL, FailIfLockfileIsNULL) 
{
    CNameIdMap name_id_map;

    int ret = name_id_map.Init(_lock_file, _caller_table_file);
    ASSERT_NE(0, ret );

    ret = name_id_map.GetID();
    ASSERT_GT(0, ret );

    caller_name_t caller_info;
    ret = name_id_map.GetCallerInfo(1, caller_info);
    ASSERT_NE(0, ret );

}

////////////////////////////////////////////////////////////
/**
 * lock_file非NULL, caller_table_file为NULL的CNameIdMap测试
 *
 */
class CNameIdMapTestWithCallerTablefileIsNULL
    : public testing::Test
{
    public:
        CNameIdMapTestWithCallerTablefileIsNULL()
            : _lock_file(NULL), _caller_table_file(NULL)
        {
            _lock_file = new char[1024];
            strncpy( _lock_file, "/tmp/caller_table.lock", 1023);
        }
        virtual ~CNameIdMapTestWithCallerTablefileIsNULL()
        {
            delete [] _lock_file;
        }

    protected:
        virtual void SetUp()
        {
        }
        virtual void TearDown()
        {
        }

        char *_lock_file;
        char *_caller_table_file;
};

/**
 * lock_file非NULL, caller_table_file为NULL时，Init/GetID/GetCallerInfo方法执行失败
 */
TEST_F(CNameIdMapTestWithCallerTablefileIsNULL, FailIfCallerTablefileIsNULL) 
{
    CNameIdMap name_id_map;

    int ret = name_id_map.Init(_lock_file, _caller_table_file);
    ASSERT_NE(0, ret );

    ret = name_id_map.GetID();
    ASSERT_GT(0, ret );

    caller_name_t caller_info;
    ret = name_id_map.GetCallerInfo(1, caller_info);
    ASSERT_NE(0, ret );

}

////////////////////////////////////////////////////////////
/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都不存在情况下的CNameIdMap测试
 *
 */
class CNameIdMapTestWithBothfileNotExist
    : public testing::Test
{
    public:
        CNameIdMapTestWithBothfileNotExist()
            : _lock_file(NULL), _caller_table_file(NULL)
        {
            _lock_file = new char[1024];
            strncpy( _lock_file, "/tmp/caller_table.lock", 1023);

            _caller_table_file = new char[1024];
            strncpy( _caller_table_file, "/tmp/caller_table.bin", 1023);
        }
        virtual ~CNameIdMapTestWithBothfileNotExist()
        {
            delete [] _lock_file;
            delete [] _caller_table_file;
        }

    private:
        int InitLocalName()
        {       
            int pid = getpid();

            char proc_exe[1024];
            snprintf( proc_exe, 1024, "/proc/%u/exe", pid );
            int len = readlink( proc_exe, _local_path, NAME_BUFFER_SIZE );
            if( len < 0){
                return -2;
            }
            _local_path[len] = '\0';

            char proc_cmdline[1024];
            snprintf( proc_cmdline, 1024, "/proc/%u/cmdline", pid);

            int fd = open( proc_cmdline, O_RDONLY );
            if( fd < 0 ) {
                return -3;
            }   
            len = read(fd, _local_param, sizeof(_local_param)-1);
            if( len < 0 ) {
                close(fd);
                return -4;
            }
            close(fd);
            for( int i = 0; i < len; i++)
            {
                if( _local_param[i] == '\0' )
                {
                    _local_param[i] = ' ';
                }
            }
            _local_param[len] = '\0';

            return 0;
        }

    protected:
        virtual void SetUp()
        {
            CFileUtility::RmFile( _lock_file );
            CFileUtility::RmFile( _caller_table_file );

            InitLocalName();
        }
        virtual void TearDown()
        {
            CFileUtility::RmFile( _lock_file );
            CFileUtility::RmFile( _caller_table_file );
        }

        char *_lock_file;
        char *_caller_table_file;

        char _local_path[512];
        char _local_param[512];
};

/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都不存在情况下, Init/GetID/GetCallerInfo方法执行成功
 */
TEST_F(CNameIdMapTestWithBothfileNotExist, SuccessIfBothfileNotExist) 
{
    bool exists = CFileUtility::IsExist( _lock_file );
    ASSERT_EQ(false, exists);

    exists = CFileUtility::IsExist( _caller_table_file );
    ASSERT_EQ(false, exists);

    CNameIdMap name_id_map;

    int ret = name_id_map.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id = name_id_map.GetID();
    ASSERT_EQ(1, id);

    int tmp_id = name_id_map.GetID();
    ASSERT_EQ(id, tmp_id);

    caller_name_t caller_info;
    ret = name_id_map.GetCallerInfo(id, caller_info);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info._path );
    ASSERT_STREQ( _local_param, caller_info._param);

}

/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都不存在情况下, 多个CNameIdMap对象时，Init/GetID/GetCallerInfo方法执行成功
 */
TEST_F(CNameIdMapTestWithBothfileNotExist, SuccessWhenMutiCNameIdMapObject) 
{
    bool exists = CFileUtility::IsExist( _lock_file );
    ASSERT_EQ(false, exists);

    exists = CFileUtility::IsExist( _caller_table_file );
    ASSERT_EQ(false, exists);

    CNameIdMap name_id_map1;

    int ret = name_id_map1.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id1 = name_id_map1.GetID();
    ASSERT_EQ(1, id1);

    caller_name_t caller_info1;
    ret = name_id_map1.GetCallerInfo(id1, caller_info1);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info1._path );
    ASSERT_STREQ( _local_param, caller_info1._param);

    CNameIdMap name_id_map2;

    ret = name_id_map2.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id2 = name_id_map2.GetID();
    ASSERT_EQ(1, id2);

    caller_name_t caller_info2;
    ret = name_id_map2.GetCallerInfo(id2, caller_info2);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info2._path );
    ASSERT_STREQ( _local_param, caller_info2._param);

    // check different CNameIdMap objects's value
    ASSERT_STREQ( caller_info1._path, caller_info2._path );
    ASSERT_STREQ( caller_info1._param, caller_info2._param);


}


////////////////////////////////////////////////////////////
/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都存在情况下的CNameIdMap测试
 *
 */
class CNameIdMapTestWithBothfileExist
    : public testing::Test
{
    public:
        CNameIdMapTestWithBothfileExist()
            : _lock_file(NULL), _caller_table_file(NULL)
        {
            _lock_file = new char[1024];
            strncpy( _lock_file, "/tmp/caller_table.lock", 1023);

            _caller_table_file = new char[1024];
            strncpy( _caller_table_file, "/tmp/caller_table.bin", 1023);
        }
        virtual ~CNameIdMapTestWithBothfileExist()
        {
            delete [] _lock_file;
            delete [] _caller_table_file;
        }

    private:
        int InitLocalName()
        {       
            int pid = getpid();

            char proc_exe[1024];
            snprintf( proc_exe, 1024, "/proc/%u/exe", pid );
            int len = readlink( proc_exe, _local_path, NAME_BUFFER_SIZE );
            if( len < 0){
                return -2;
            }
            _local_path[len] = '\0';

            char proc_cmdline[1024];
            snprintf( proc_cmdline, 1024, "/proc/%u/cmdline", pid);

            int fd = open( proc_cmdline, O_RDONLY );
            if( fd < 0 ) {
                return -3;
            }   
            len = read(fd, _local_param, sizeof(_local_param)-1);
            if( len < 0 ) {
                close(fd);
                return -4;
            }
            close(fd);
            for( int i = 0; i < len; i++)
            {
                if( _local_param[i] == '\0' )
                {
                    _local_param[i] = ' ';
                }
            }
            _local_param[len] = '\0';

            return 0;
        }


    protected:
        virtual void SetUp()
        {
            CFileUtility::TouchFile( _lock_file );
            CFileUtility::TouchFile( _caller_table_file );

            InitLocalName();
        }
        virtual void TearDown()
        {
            CFileUtility::RmFile( _lock_file );
            CFileUtility::RmFile( _caller_table_file );
        }

        char *_lock_file;
        char *_caller_table_file;

        char _local_path[512];
        char _local_param[512];
};

/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都存在情况下, Init/GetID/GetCallerInfo方法执行成功
 */
TEST_F(CNameIdMapTestWithBothfileExist, SuccessIfBothfileExist) 
{
    bool exists = CFileUtility::IsExist( _lock_file );
    ASSERT_EQ(true, exists);

    exists = CFileUtility::IsExist( _caller_table_file );
    ASSERT_EQ(true, exists);

    CNameIdMap name_id_map;

    int ret = name_id_map.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id = name_id_map.GetID();
    ASSERT_EQ(1, id);

    int tmp_id = name_id_map.GetID();
    ASSERT_EQ(id, tmp_id);

    caller_name_t caller_info;
    ret = name_id_map.GetCallerInfo(id, caller_info);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info._path );
    ASSERT_STREQ( _local_param, caller_info._param);

}
/**
 * lock_file非NULL, caller_table_file非NULL,
 * 但是这两个文件都存在情况下, 多个CNameIdMap对象时，Init/GetID/GetCallerInfo方法执行成功
 */
TEST_F(CNameIdMapTestWithBothfileExist, SuccessWhenMutiCNameIdMapObject) 
{
    bool exists = CFileUtility::IsExist( _lock_file );
    ASSERT_EQ(true, exists);

    exists = CFileUtility::IsExist( _caller_table_file );
    ASSERT_EQ(true, exists);

    CNameIdMap name_id_map1;

    int ret = name_id_map1.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id1 = name_id_map1.GetID();
    ASSERT_EQ(1, id1);

    caller_name_t caller_info1;
    ret = name_id_map1.GetCallerInfo(id1, caller_info1);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info1._path );
    ASSERT_STREQ( _local_param, caller_info1._param);

    CNameIdMap name_id_map2;

    ret = name_id_map2.Init(_lock_file, _caller_table_file);
    ASSERT_EQ(0, ret );

    int id2 = name_id_map2.GetID();
    ASSERT_EQ(1, id2);

    caller_name_t caller_info2;
    ret = name_id_map2.GetCallerInfo(id2, caller_info2);
    ASSERT_EQ(0, ret );

    ASSERT_STREQ( _local_path, caller_info2._path );
    ASSERT_STREQ( _local_param, caller_info2._param);

    // check different CNameIdMap objects's value
    ASSERT_STREQ( caller_info1._path, caller_info2._path );
    ASSERT_STREQ( caller_info1._param, caller_info2._param);


}


