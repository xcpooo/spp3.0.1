#ifndef _SPP_COMM_MISC_H_
#define _SPP_COMM_MISC_H_
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#define MAXPATHLEN 1024

namespace spp
{
    namespace comm
    {
        class CMisc
        {
        public:
            static unsigned getip(const char* ifname);
            static unsigned getmemused();
            
            // 判断进程是否存在？
            // 返回值： 0 - 不存在； 1 - 存在
            static int check_process_exist(pid_t pid);
            static inline int64_t time_diff(const struct timeval &tv1, const struct timeval &tv2)
            {
                return ((int64_t)(tv1.tv_sec - tv2.tv_sec - 1))*1000 + (1000000 + tv1.tv_usec - tv2.tv_usec)/1000;
            }
            // realloc safe check version 20140523
            static inline void* realloc_safe(void *ptr, size_t size) 
            {
                void* new_ptr = realloc(ptr, size);
                if (NULL == new_ptr) {
                    free(ptr);
                } 
                return new_ptr;
            };

        };
    }
}
#endif
