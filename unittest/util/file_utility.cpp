/*
 * =====================================================================================
 *
 *       Filename:  file_utility.cpp
 *
 *    Description:  文件操作封装
 *
 *        Version:  1.0
 *        Created:  11/23/2010 05:03:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "file_utility.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void CFileUtility::TouchFile(const char* file)
{
    char szTouchCmd[1500];
    snprintf( szTouchCmd, sizeof(szTouchCmd), "touch %s", file);
    system( szTouchCmd );
}

void CFileUtility::RmFile(const char* file)
{
    char szRmCmd[1500];
    snprintf( szRmCmd, sizeof(szRmCmd), "rm -f %s", file);
    system( szRmCmd );
}

bool CFileUtility::IsExist(const char* file)
{
    struct stat st;
    int ret = stat(file, &st);
    if( ret == 0 )
    {
        return true;
    }
    
    return false;
}

bool CFileUtility::TruncateFile(const char*file, unsigned int size)
{
    TouchFile(file);
    int fd = open(file, O_RDWR, 0666);
    if(fd >= 0)
    {
        ftruncate(fd, size); 

        close(fd);
        return true;
    }

    return false;
}
