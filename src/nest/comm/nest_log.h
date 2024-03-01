/**
 * @brief nest log define 
 */

#ifndef __NEST_LOG_H_
#define __NEST_LOG_H_

#include "global.h"
#include "tlog.h"
#include "singleton.h"

using namespace tbase::tlog;
using namespace spp::singleton;

/**
 * @brief 鸟巢日志宏适配定义, 默认实现是获取spp框架日志接口
 */
#define NEST_LOG(lvl, fmt, args...)                                         \
do {                                                                        \
    register CTLog* flog = INTERNAL_LOG;                                    \
    if (flog && (lvl >= flog->log_level(-1))) {                             \
        flog->LOG_P_ALL(lvl, fmt"\n", ##args);                              \
    }                                                                       \
} while (0)


#endif


