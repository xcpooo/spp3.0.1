/*
 * =====================================================================================
 *
 *       Filename:  stat.cpp
 *
 *    Description:
 *
 *        Version:  1.1
 *        Created:  Thu Dec 17 11:22:29 CST 2009
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jeremychen(for fun), jeremychen@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include "tstat.h"

#define STAT_BUF_SIZE 1<<14

using namespace tbase::tstat;

//added by aoxu,2009-12-29
const char* STAT_TOOL_VERSION = "1.2.0";

int main(int argc, char** argv)
{
	if(argc < 2) {
		printf("usage: %s ../stat/stat_spp_proxy.stat\n", argv[0]);
		return 0;
	}
	if(!strncmp(argv[1], "-v", 2)) {
		printf("spp stat tool ver: %s\n", STAT_TOOL_VERSION);
		return 0;
	}
    CTStat st;
    int ret = st.read_statpool(argv[1]);
    if (ret == ERR_STAT_OPENFILE) {
        printf("stat file[%s] open error.\n", argv[1]);
        return -1;
    } else if (ret == ERR_STAT_TRUNFILE) {
        printf("stat file[%s] not exist.\n", argv[1]);
        return -2;
    } else if (ret == ERR_STAT_MAPFILE) {
        printf("stat file[%s] mmap error.\n", argv[1]);
        return -3;
    } else if (ret == ERR_STAT_MEMERROR) {
        printf("stat file[%s] mem error.\n", argv[1]);
        return -4;
    } else if (ret == ERR_STAT_FILE_NULL) {
		printf("stat file cannot be NULL.\n");
		return -5;
	}

    char*p = new char[STAT_BUF_SIZE];
    if(p == NULL)
    {
		printf("stat buf malloc failed.\n");
		return -6;
    }
    int len = 1024 * 16;
    st.result(&p, &len, STAT_BUF_SIZE);
    printf("the length is %d\n", len);

    if (len > 102400) {
        printf("the buffer length is too small (length is %d)", len);
        delete [] p; 
        return -7;
    }

    printf("%s", p);
    delete [] p; 
    return 0;
}

