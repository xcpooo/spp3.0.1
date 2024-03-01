#ifndef _SPP_GLOBAL_H
#define _SPP_GLOBAL_H
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <tbase/tlog.h>

#define  INTERNAL_LOG  SingleTon<CTLog, CreateByProto>::Instance()

//class tbase::tlog::CTLog;
namespace spp
{

    namespace global
    {
        class spp_global
        {
        public:
            static void daemon_set_title(const char* fmt, ...);
			//拷贝参数
			static void save_arg(int argc, char** argv);
            //static void save_log_info(std::string& path,std::string& prefix,std::string& level,int size);
            static char* arg_start;
            static char* arg_end;
            static char* env_start;
            /*
            static std::string log_path;
            static std::string sb_log_prefix;
            static int sb_log_level;
            static int log_size;
            */
			//共享内存满的告警脚本
            static std::string mem_full_warning_script;
            //当前proxy处理那个group的共享内存
            static int cur_group_id;

            static void set_cur_group_id(int group_id);
            static int get_cur_group_id();

            // add global config info
            static int link_timeout_limit;

        };

    }


}






#endif
