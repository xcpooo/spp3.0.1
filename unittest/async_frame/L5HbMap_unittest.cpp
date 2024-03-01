/*
 * =====================================================================================
 *
 *       Filename:  L5HbMap_unittest.cpp
 *
 *    Description:  CHBMap单元测试
 *
 *        Version:  1.0
 *        Created:  11/25/2010 06:04:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <unistd.h>
#include <file_utility.h>
#include <async_frame/L5HbMap.h>

USING_L5MODULE_NS;

#define MAPFILE "/tmp/l5hbmapfile.bin"

/**
 * 测试CHBMap时使用的参数准备
 *
 */
class CHBMapFile
{
    public:
        CHBMapFile()
        {
            snprintf(_mapfile, 1024, "%s", MAPFILE );
        }
        ~CHBMapFile()
        {
            CFileUtility::RmFile( _mapfile );
        }


        char *GetNULLFile()
        {
            return NULL;
        }

        char *GetNotExistFile()
        {
            CFileUtility::RmFile( _mapfile );
            return _mapfile;
        }

        char *GetExistButNotEnoughSizeFile()
        {
            CFileUtility::TouchFile( _mapfile );
            return _mapfile;
        }

        char *GetExistAndEnoughSizeFile()
        {
            CFileUtility::TruncateFile( _mapfile, sizeof(hb_map_t));
            return _mapfile;
        }

    protected:
        char _mapfile[1024];
};

/**
 * 测试CHBMap::InitMap
 */
TEST(CHBMapTest, InitMap)
{
    CHBMapFile mapfile;

    hb_map_t *hb_map = CHBMap::InitMap( mapfile.GetNULLFile() );
    EXPECT_EQ( 0, (long long)hb_map ) << "CHBMap::InitMap in the condition of mapfile == NULL";

    hb_map = CHBMap::InitMap( mapfile.GetNotExistFile() );
    EXPECT_EQ( 0, (long long)hb_map ) << "CHBMap::InitMap in the condition of  mapfile not exist";

    hb_map = CHBMap::InitMap( mapfile.GetExistButNotEnoughSizeFile());
    EXPECT_EQ( 0, (long long)hb_map ) << "CHBMap::InitMap in the condition of  mapfile exist but have not enough size";

    hb_map = CHBMap::InitMap( mapfile.GetExistAndEnoughSizeFile());
    EXPECT_NE( 0, (long long)hb_map ) << "CHBMap::InitMap in the condition of  mapfile exist and have enough size";

}
