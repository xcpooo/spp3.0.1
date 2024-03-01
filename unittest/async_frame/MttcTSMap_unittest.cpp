/*
 * =====================================================================================
 *
 *       Filename:  MttcTSMap_unittest.cpp
 *
 *    Description:  CMttcTSMap单元测试
 *
 *        Version:  1.0
 *        Created:  11/25/2010 08:38:16 PM
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
#include <async_frame/MttcTSMap.h>

USING_L5MODULE_NS;

#define MAPFILE "/tmp/mttc_ts.bin"

/**
 * 为CMttcTSMap测试准备使用的file参数
 */
class CTSMapFile
{
    public:
        CTSMapFile()
        {
            snprintf(_mapfile, 1024, "%s", MAPFILE );
        }
        ~CTSMapFile()
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
            CFileUtility::TruncateFile( _mapfile, sizeof(ts_map_t));
            return _mapfile;
        }

    protected:
        char _mapfile[1024];
};


/**
 * 测试CMttcTSMap::InitMap
 *
 */
TEST(CMttcTSMapTest, InitMap)
{
    CTSMapFile mapfile;

    ts_map_t *ts_map = CMttcTSMap::InitMap( mapfile.GetNULLFile() );
    EXPECT_EQ( 0, (long long)ts_map ) << "CMttcTSMap::InitMap in the condition of mapfile == NULL";

    ts_map = CMttcTSMap::InitMap( mapfile.GetNotExistFile() );
    EXPECT_EQ( 0, (long long)ts_map ) << "CMttcTSMap::InitMap in the condition of  mapfile not exist";

    ts_map = CMttcTSMap::InitMap( mapfile.GetExistButNotEnoughSizeFile());
    EXPECT_EQ( 0, (long long)ts_map ) << "CMttcTSMap::InitMap in the condition of  mapfile exist but have not enough size";

    ts_map = CMttcTSMap::InitMap( mapfile.GetExistAndEnoughSizeFile());
    EXPECT_NE( 0, (long long)ts_map ) << "CMttcTSMap::InitMap in the condition of  mapfile exist and have enough size";

}
