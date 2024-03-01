/*
 * =====================================================================================
 *
 *       Filename:  file_utility.h
 *
 *    Description:  文件操作封装
 *
 *        Version:  1.0
 *        Created:  11/23/2010 05:03:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __FILE_UTILITY_H__
#define __FILE_UTILITY_H__


class CFileUtility
{
    public:
        static void TouchFile(const char* file);
        static void RmFile(const char* file);
        static bool IsExist(const char* file);
        static bool TruncateFile(const char*file, unsigned int size);
};

#endif
