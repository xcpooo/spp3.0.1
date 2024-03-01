/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  04/12/2009 06:41:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  allanhuang (for fun), allanhuang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "keygen.h"
#include "ctrlconf.h"
#include "workerconf.h"

#define COLOR_GREEN "\e[1m\e[32m"
#define COLOR_RED   "\e[1m\e[31m"
#define COLOR_END   "\e[m"
int error = 0;
using namespace std;
using namespace spp::comm;
using namespace spp::tconfbase;

typedef struct {
    int id;
    int recv_key;
    int send_key;
} worker_info;
vector<worker_info> v_worker_info;

int main(int argc , char** argv)
{
    if (argc < 2) {
        printf("usage:%s ../etc/spp_ctrl.xml\n", argv[0]);
        exit(1);
    }

    printf("%s%s%s", COLOR_RED, "\n!!!!注意：这个工具是用目录来计算shm key和mq key,所以必须在bin目录下运行，否则取到的key是不正确的!\n", COLOR_END);
    printf("%s%s%s", COLOR_RED, "!!!!ATTENTION:This tool is used to calc shm key and mq key by CWD, \nso it must be run under diractory \"bin\",or the key is no significance!\n\n", COLOR_END);
    //	system((char*)"rm -rf /tmp/mq_*.lock");
    unsigned mqkey = pwdtok(255);
    
    CTLog flog;
    SingleTon<CTLog, CreateByProto>::SetProto(&flog);

    printf("MQ_KEY%s[0x%x]%s\n", COLOR_GREEN, mqkey, COLOR_END);


    CCtrlConf conf(new CLoadXML(argv[1]));
    if (conf.init() != 0)
    {
        return -1;
    }


    Procmon proc;
    conf.getAndCheckProcmon(proc);

    for (size_t i = 0; i < proc.entry.size(); ++ i)
    {
        string conf_file = proc.entry[i].etc;
        
        CLoadXML worker(conf_file) ;
        if (worker.loadConfFile() != 0)
        {
            fprintf(stderr, "Load xml %s failed\n", conf_file.c_str());
            return -1;
        }
        if (proc.entry[i].id == 0)
            continue;

        unsigned k1 = pwdtok(2 * proc.entry[i].id);
        unsigned k2 = pwdtok(2 * proc.entry[i].id + 1);
        printf("worker%s[%d]%s RECV_KEY%s[0x%x]%s SEND_KEY%s[0x%x]%s \n", 
                    COLOR_GREEN, proc.entry[i].id, COLOR_END, COLOR_GREEN, k1, COLOR_END, COLOR_GREEN, k2, COLOR_END);


    }

    printf("\n");
    return 0;
}

